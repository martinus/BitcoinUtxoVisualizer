=begin
CBlock::SerializationOp() in block.h
    READWRITEAS(CBlockHeader, *this);
        READWRITE(this->nVersion);  // int32_t nVersion;
        READWRITE(hashPrevBlock); // uint256 hashPrevBlock
            s.write((char*)data, sizeof(data));
        READWRITE(hashMerkleRoot); // uint256 hashMerkleRoot
            s.write((char*)data, sizeof(data));
        READWRITE(nTime); // uint32_t nTime
        READWRITE(nBits); // uint32_t nBits
        READWRITE(nNonce); // uint32_t nNonce
       
    READWRITE(vtx); // std::vector<CTransactionRef> vtx
        WriteCompactSize(os, v.size()); // see serialize.h "void WriteCompactSize(Stream& os, uint64_t nSize)"
        // loop serializes CTransaction, in transaction.h "inline void SerializeTransaction(const TxType& tx, Stream& s) {"
            s << tx.nVersion;


=end
File.open(ARGV[0], "rb") do |f|

end