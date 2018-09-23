:: see https://trac.ffmpeg.org/wiki/Encode/H.264
::
:: creates a losslessly compressed x264 video. It is quite compact
:: as only few pixels change from frame to frame.
::3rdparty\ffmpeg -r 60 -i img_with_legend\%%08d.png -c:v libx264 -preset veryslow -crf 0 utxo_with_legend.mp4
3rdparty\ffmpeg -r 60 -i img_with_legend\%%08d.png -pix_fmt yuv444p -profile:v high444 -c:v libx264 -preset veryslow -crf 0 utxo_with_legend.mp4


pause