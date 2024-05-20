import Controller_interface
from Controller_interface import SMCBaseMotorController
import redis, json
import os
import time
import struct
import pickle
import time



redis_host = 'localhost'
redis_port = 6379

motors_container = redis.Redis(host=redis_host, port=redis_port, db = 0)

DataPipe = r'\\.\pipe\DataPipe'
MotorPipe = r'\\.\pipe\MotorPipe'
    
def motor_calibration(size):
    #X
    motor = pickle.loads(motors_container.get('photo_motor'))
    motor.StartOne(1, 0);
    while(motor.smc100.getPosition(1) != 0):
        motor_pipe(-3)
        time.sleep(0.1)
    motor.StartOne(1, 250);
    motor_pipe(-1)
    while(motor.smc100.getPosition(1) < 248):
        if(photo_pipe(DataPipe) == -1):
            pos = motor.smc100.getPosition(1)
            if(pos is None):
                motor_pipe(-2)
            else:
                motor_pipe(pos)
    xPos = photo_pipe(DataPipe)
    motor.StartOne(1, xPos)
    
    #Y
    
    motor.StartOne(2, 0);
    while(motor.smc100.getPosition(2) != 0):
        motor_pipe(-3)
        time.sleep(0.1)
    motor.StartOne(2, 250);
    motor_pipe(-1)
    while(motor.smc100.getPosition(2) < 248):
        if(photo_pipe(DataPipe) == -1):
            pos = motor.smc100.getPosition(2)
            if(pos is None):
                motor_pipe(-2)
            else:
                motor_pipe(pos)

    yPos = photo_pipe(DataPipe)
    motor.StartOne(2, yPos)

    #Motors
    keys = motors_container.keys('*')
    print(keys, '\n')
    for key in keys:
        motor = pickle.loads(motors_container.get(key))
        motor.StartOne(1, xPos - size[0])
        motor.StartOne(2, yPos - size[1])

        

def photo_pipe(pipe_name):
    while not os.path.exists(pipe_name):
        time.sleep(0.01)

    with open(pipe_name, 'rb') as pipe:
        data = pipe.read(4)  
        number = struct.unpack('i', data)[0]
        print(f"Received number: {number}")
    return number
        
def motor_pipe(pos):
    while not os.path.exists(MotorPipe):
        print("Waiting for the pipe to be created...")
        time.sleep(0.01)
    
    with open(MotorPipe, 'rb') as pipe:
        message = struct.pack('i', pos)
        pipe.write(message)
        print("Message sent from Python")