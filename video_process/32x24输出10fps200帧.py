import numpy as np
import cv2 as cv
import output
#https://blog.csdn.net/wfei101/article/details/82024877
max_frame = 200
fps=10
width=32
height=8

out_path1='./0-0-0-0.txt'
out_path2='./0-0-0-1.txt'
out_path3='./0-1-0-0.txt'

#需要处理的视频（建议提前处理成4：3比例）路径填入下方：
vc = cv.VideoCapture('./"videoName".mp4')

#https://www.cnblogs.com/ycycn/p/14456848.html
fps_old=vc.get(5)#原视频的fps
fcount=vc.get(7)#原视频的总帧数
size_w=vc.get(3)
size_h=vc.get(4)

#print(fps_old)
#print(size_w)
#print(size_h)

ratio=int(fps_old/fps)#允许一定的播放速度误差
ratio=max(1,ratio)
count=0#原视频中帧数统计
count1=-1#新视频中帧数统计
fo1 = open(out_path1, "w")
fo2 = open(out_path2, "w")
fo3 = open(out_path3, "w")

while(vc.isOpened()):
    ret, frame = vc.read()
    count=count+1
    if not ret or count1>=max_frame:#读到结束 
        break
    if not count%ratio==0:
        continue
    count1=count1+1
    
    rgb = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
    rgb1=rgb[int(0):int(size_h/3),0:int(size_w)]
    rgb2=rgb[int(size_h/3):int(2.0*size_h/3.0),0:int(size_w)]
    rgb3=rgb[int(2.0*size_h/3.0):int(size_h),0:int(size_w)]

    rgb1=cv.resize(rgb1,(width,height),interpolation=cv.INTER_NEAREST)
    rgb2=cv.resize(rgb2,(width,height),interpolation=cv.INTER_NEAREST)
    rgb3=cv.resize(rgb3,(width,height),interpolation=cv.INTER_NEAREST)

    output.led(fo1,rgb1)
    output.led(fo2,rgb2)
    output.led(fo3,rgb3)
    """
    本段代码用于预览效果
    display=cv.cvtColor(rgb, cv.COLOR_RGB2BGR)
    display=cv.resize(display,(width,height*3),interpolation=cv.INTER_NEAREST)
    display=cv.resize(display,(320,240),interpolation=cv.INTER_NEAREST)
    cv.imshow('frame',display)
    if cv.waitKey(100) & 0xFF == ord('q'):
		break
	"""
vc.release()
fo1.close()
fo2.close()
fo3.close()

#cv.destroyAllWindows()
