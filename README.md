# FFmpegDemo
实现了简单直播功能  
功能和技术：  
### 推流  
  
  1、利用android的摄像头采集图像信息，传入native层用x264进行封包，再用rtmp进行封包推流   
  2、利用android的录音API采集音频信息，传入native层用faac进行封包，再用rtmp进行封包推流   
  
### 接收流/播放视频  
  
  1、视频：在native层用ffmpeg进行解码，将解码后的图像信息绘制到android的surfaceView   
  2、音频：在native层用ffmpeg进行解码，将解码后的音频信息利用android集成的openSL ES进行播放   
