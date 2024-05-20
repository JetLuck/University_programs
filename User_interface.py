import time
import pickle
import Controller_interface
from Controller_interface import SMCBaseMotorController
import Calibration
import redis
import json

message = {'cmd' : '',
           'motor' : '',
           'arg1' : 0,
           'arg2' : 0}

redis_host = 'localhost'
redis_port = 6379
controller_commands_channel = 'controller_commands'
controller_output_channel = 'controller_output'

motors_container = redis.Redis(host=redis_host, port=redis_port, db = 0)

controller_commands_redis = redis.StrictRedis(host=redis_host, port=redis_port, db=0)
controller_commands_pubsub = controller_commands_redis.pubsub()
controller_commands_pubsub.subscribe(controller_commands_channel)

controller_output_redis = redis.StrictRedis(host=redis_host, port=redis_port, db=0)
controller_output_pubsub = controller_output_redis.pubsub()
controller_output_pubsub.subscribe(controller_output_channel)

def listen_from_controller():
    time.sleep(1)
    message = controller_output_pubsub.parse_response()
    if message:
        if message[0] == "Unknown":
            print("Unknown command, try help")
        elif message[0].decode('utf-8') == 'subscribe':
            return
        else:
            print(message)

if __name__ == "__main__":
    controller_output_pubsub.parse_response()
    while(1):
        data = input("Print the command: ").lower().split(' ')
        print(data)
        if (data[0] == "help"):
               print("List of commands and args:")
               print("Load             [\"filename\"]")
               print("Save             [\"filename\"]")
               print("Create           [\"family\\domain\\name\"]")
               print("Add              [motor, axis]")
               print("Delete           [motor, axis]")
               print("Pos              [motor, axis]")
               print("State            [motor, axis]")
               print("Start            [motor, axis]")
               print("Stop             [motor, axis]")
               print("Status           [motor, axis]")
               print("Calibrate        [diod_motor, size]")
        elif (data[0] == 'load'):
            try:
                with open(data[1], "r") as file:
                    for line in file:
                        line = line.split(' ');
                        if line[0] == 'motor':
                            message['cmd'] = 'create'
                            motor_name = line[1]
                            message['motor'] = line[1]
                            json_pub = json.dumps(message)
                            controller_commands_redis.publish(controller_commands_channel,json_pub)
                            listen_from_controller()
                        elif line[0] == 'axis':
                            message['cmd'] = 'add'
                            message['motor'] = motor_name
                            message['arg1'] = line[1]
                            json_pub = json.dumps(message)
                            controller_commands_redis.publish(controller_commands_channel,json_pub)
                            listen_from_controller()
                            message['cmd'] = 'start'
                            message['motor'] = motor_name
                            message['arg1'] = line[1]
                            message['arg2'] = line[2]
                        json_pub = json.dumps(message)
                        controller_commands_redis.publish(controller_commands_channel,json_pub)
                        listen_from_controller()
            except FileNotFoundError:
                print("File not found")
            except IOError:
                print("Input - output error")
        elif (data[0] == 'save'):
            with open(data[1], 'w') as file:
                keys = motors_container.keys('*')
                print(keys, '\n')
                for key in keys:
                    motor = pickle.loads(motors_container.get(key))
                    file.write('motor ', motor.name, '\n')
                    print(motor.attributes)
                    for axis in motor.attributes:
                        file.write('axis ', axis, motor.smc100.getPosition(axis), "\n")
                        print(motor.name, axis, motor.smc100.getPosition(axis), " saved\n")
                    print("data saved!\n")
        elif(data[0] == 'calibrate'):
            message['cmd'] = 'create'
            message['motor'] = 'photo_motor'
            json_pub = json.dumps(message)
            controller_commands_redis.publish(controller_commands_channel,json_pub)
            listen_from_controller()
            Calibration.motor_calibration(int(data[1])/2)
        else:
            if (len(data) > 1):
                message['cmd'] = data[0]
                message['motor'] = data[1]
                if (len(data) == 3):
                    message['arg1'] = data[2]
                else:
                    message['arg1'] = ''
                if (len(data) == 4):
                    message['arg2'] = data[3]
                else:
                    message['arg2'] = ''

                json_pub = json.dumps(message)
                controller_commands_redis.publish(controller_commands_channel,json_pub)
                listen_from_controller()
            else:
                print("Not enough arguments")
        
        


         