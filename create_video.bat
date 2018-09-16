:: see https://trac.ffmpeg.org/wiki/Encode/H.264

C:\Users\martinus\Downloads\ffmpeg-20180914-6304268-win64-static\bin\ffmpeg -r 60 -i img\%%05d.png -c:v libx264 -preset veryslow -crf 0 output.mkv
::C:\Users\martinus\Downloads\ffmpeg-20180914-6304268-win64-static\bin\ffmpeg -r 60 -i img\%%05d.png -c:v libx264 -preset ultrafast -crf 0 output.mkv

pause