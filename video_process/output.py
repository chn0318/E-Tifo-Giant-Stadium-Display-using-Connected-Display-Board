import cv2 as cv
def outputfile(file, frame):
    for i in range(0,8):
        for j in range(0,8):#i是行,j是列
            if(frame[i,j]==255):
                file.write(str(0))
            else:
                file.write(str(1))

def outputshowfile(file, frame):
    for i in range(0,8):
        for j in range(0,8):#i是行,j是列
            if(frame[i,j]==255):
                #file.write(str(1))
                file.write('x')
            else:
                #file.write(str(0))
                file.write(' ')
        file.write('\n')
    file.write('\n')

def led(file, frame):
    for i in range(0,16):#第i个来回
        for j in range(0,16):
            tmpa=hex(frame[int(abs(j-7.5)-0.5),32-2*i-1-j//8,0])[2:4]
            tmpb=hex(frame[int(abs(j-7.5)-0.5),32-2*i-1-j//8,1])[2:4]
            tmpc=hex(frame[int(abs(j-7.5)-0.5),32-2*i-1-j//8,2])[2:4]

            if(len(tmpa)==1):
                file.write('0')
            file.write(tmpa)
            if(len(tmpb)==1):
                file.write('0')
            file.write(tmpb)
            if(len(tmpc)==1):
                file.write('0')
            file.write(tmpc)

