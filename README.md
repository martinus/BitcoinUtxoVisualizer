# BitcoinUtxoVisualizer

Parses the whole blockchain with the use of 
[Bitcoin's REST interface](https://github.com/bitcoin/bitcoin/blob/master/doc/REST-interface.md).

It can be used to generate a video of the evolution of the UTXO.

The most recent (and largest) video that I've done is this: https://www.youtube.com/watch?v=sT_SOEc_U_A

It also flashes the transactions of the latest block as white dots (so basically the UTXO's from the past that are removed)

The basic workflow is like this:

1. First, fetch data from bitcoin full node (using the JSON interface), and preprocess this into a binary data dump. This is done with `utxo_to_change.rb`. It is a timeconsuming process. Could probably be optimized by using a different API or directly operating on the binary data.

1. After generating the binary dump, an C++ generator (BitcoinVisualizer) parses the dump and generates images that can be directly piped into `ffmpeg` which generates a video. Most of the magic is done in the class `Density`. This creates a socket connection, and dumps each image it generates from the binary data into the socket for `ffmpeg` to process. Note that currently it's hardcoded to stop at block 200000. Instead of `ffmpeg` you can also dump the images into `ffplay` to directly visualize the output.

Currently the C++ code is a bit platform specific unfortunately, and not well documented.
