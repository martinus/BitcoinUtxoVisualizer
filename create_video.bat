:: see https://trac.ffmpeg.org/wiki/Encode/H.264
3rdparty\ffmpeg -r 60 -i img\%%08d.png -c:v libx264 -preset veryslow -crf 0 utxo.mp4

pause