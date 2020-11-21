#include "Utxo.h"

#include <util/BinaryStreamReader.h>
#include <util/BinaryStreamWriter.h>
#include <util/Mmap.h>

#include <fstream>

// data format of the Utxo:
//
// field size | description      | data type   | commment
// -----------|------------------|-------------|-------
//          8 | utxo_map_size    | size_t      | number of entries in the Utxo map
// The following entries are repeated utxo_map_size times:
//         16 | TxId             | uint8_t[16] | first 16 bytes of the TxId. That's enough to prevent collisions.
//          8 | utxo_per_tx_size | size_t      | number of unspent transactions for this tx
// The following entries are repeated utxo_per_tx_size times:
//          2 | vout_nr          | uint16_t    | vout number (65k should be plenty)
//          8 | satoshi          | uint64_t    | Amount in satoshi

namespace {

void dump(buv::Utxo const& utxo, std::filesystem::path const& filename) {
    auto fout = std::ofstream(filename, std::ios::binary | std::ios::out);
    if (!fout.is_open()) {
        throw std::runtime_error("could not open file for writing");
    }
    auto bsw = util::BinaryStreamWriter(&fout);
    bsw.write<8>(utxo.size());
    for (auto const& [txid, utxoPerTx] : utxo) {
        bsw.write<16>(txid);
        bsw.write<4>(utxoPerTx.blockHeight);
        bsw.write<8>(utxoPerTx.utxoPerTx.size());
        for (auto const& [voutNr, satoshi] : utxoPerTx.utxoPerTx) {
            bsw.write<2>(voutNr);
            bsw.write<8>(satoshi);
        }
    }
}

} // namespace

namespace buv {

auto loadUtxo(std::filesystem::path const& utxoFilename) -> Utxo { // TODO(martinus)
    auto mappedFile = util::Mmap(utxoFilename);
    if (!mappedFile.is_open()) {
        throw std::runtime_error("could not open utxo filename");
    }

    auto utxo = Utxo();

    auto bsr = util::BinaryStreamReader(mappedFile.data(), mappedFile.size());
    auto numTx = bsr.read<8, size_t>();

    utxo.reserve(numTx);
    for (size_t i = 0; i < numTx; ++i) {
        auto txid = bsr.read<16, TxId>();
        auto numUtxoPerTx = bsr.read<8, size_t>();

        auto blockheightAndUtxoPerTx = UtxoPerTx();
        blockheightAndUtxoPerTx.blockHeight = bsr.read<4, uint32_t>();
        blockheightAndUtxoPerTx.utxoPerTx.reserve(numUtxoPerTx);
        for (size_t u = 0; u < numUtxoPerTx; ++u) {
            auto voutNr = bsr.read<2, uint16_t>();
            auto satoshi = bsr.read<8, uint64_t>();
            blockheightAndUtxoPerTx.utxoPerTx.emplace(voutNr, satoshi);
        }
        // TODO(martinus) make sure this is a move!
        utxo.emplace(txid, std::move(blockheightAndUtxoPerTx));
    }

    return utxo;
}

void safeUtxo(Utxo const& utxo, std::filesystem::path const& utxoFilename) {
    static_assert(sizeof(size_t) == sizeof(uint64_t));

    // first creates a .tmp file, writes everything into it, and when that has finished successfully renames it to the actual
    // filename. That way we don't lose anything when pressing Ctrl+C while the file is written.
    auto tmpUtxoFilename = utxoFilename;
    tmpUtxoFilename += ".tmp";
    dump(utxo, tmpUtxoFilename);

    std::filesystem::rename(tmpUtxoFilename, utxoFilename);
}

} // namespace buv