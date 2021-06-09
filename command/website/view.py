from flask import Blueprint, render_template, request, flash
from .implementation import path_finder
from datetime import datetime
from .models import Items
from . import db
import websocket
import math
import time # for debugging

views = Blueprint('views',__name__)

action_log=[]
percentage=50
speed=1024
connected=False
degree=0
pathDictionary={'x': [0],'y': [0]}
dt_object = datetime.fromtimestamp(datetime.timestamp(datetime.now())).strftime("%m/%d/%Y, %H:%M:%S")

#ip="ws://81.136.3.190/" Hubert
ip= "ws://58.176.226.198:8765/"
ws = websocket.WebSocket()

def choose_longest_path(search_list):
    cur_x=pathDictionary['x'][-1]
    cur_y=pathDictionary['y'][-1]
    largest_diff=0
    largest_diff_cord=None
    for i, c in enumerate(search_list):
        for a , b in enumerate(search_list[i]):
            if search_list[i][a] != 1:
                dist_diff=math.sqrt((i-cur_y)**2+(a-cur_x)**2)
                if  dist_diff > largest_diff:
                    largest_diff=dist_diff
                    largest_diff_cord=(a,i)
    
    return largest_diff_cord

def calculate_pos(mag):
    cur_x=pathDictionary['x'][-1]
    cur_y=pathDictionary['y'][-1]
    next_x=round(cur_x+ mag * math.sin(degree/180*math.pi),1)
    next_y=round(cur_y+ mag * math.cos(degree/180*math.pi),1)
    pathDictionary['x'].append(next_x)
    pathDictionary['y'].append(next_y)

def send_action(msg):
    ws.send(msg)
    result = ws.recv()
    print("Received: " + result)
    return result

def update():
    ws.send("Update")
    result = ws.recv()
    print("Received: " + result)
    parts = result.split("-")
    return parts

def path_commands(begin,end,node_list):
    global degree
    degreeDict={(+1, 0):90, (0, -1):180, (-1, 0):270, (0, +1):0, (-1, -1):225, (-1, +1):315, (+1, -1):135, (+1, +1):45}
    x_diff=node_list[begin+1][0]-node_list[begin][0]
    y_diff=node_list[begin+1][1]-node_list[begin][1]
    desired_degree=degreeDict[(x_diff,y_diff)]
    if degree > desired_degree:
        msg="Turn Left-"+str(degree-desired_degree)
        result=send_action(msg)
        action_log.append((dt_object,"Turn Left-",result))
        degree = desired_degree  
    elif desired_degree > degree:
        msg="Turn Right-"+str(desired_degree-degree)
        result=send_action(msg)
        action_log.append((dt_object,"Turn Right-",result))
        degree = desired_degree 
    i=0
    while begin+i != end :
        x_dif=node_list[begin+i+1][0]-node_list[begin+i][0]
        y_dif=node_list[begin+i+1][1]-node_list[begin+i][1]
        desired=degreeDict[(x_dif,y_dif)]
        if desired != degree:
            break
        i += 1
    msg="Forward-"+str(round(math.sqrt(x_diff**2+y_diff**2)*i,1))
    result=send_action(msg)
    action_log.append((dt_object,"Forward-",result))
    pathDictionary['x'].append(node_list[begin+i][0])
    pathDictionary['y'].append(node_list[begin+i][1])
    return begin + i

def path_commands_v2(begin,end,node_list,search_list):
    global degree
    degreeDict={(+1, 0):90, (0, -1):180, (-1, 0):270, (0, +1):0, (-1, -1):225, (-1, +1):315, (+1, -1):135, (+1, +1):45}
    x_diff=node_list[begin+1][0]-node_list[begin][0]
    y_diff=node_list[begin+1][1]-node_list[begin][1]
    desired_degree=degreeDict[(x_diff,y_diff)]
    if degree > desired_degree:
        msg="Turn Left-"+str(degree-desired_degree)
        result=send_action(msg)
        action_log.append((dt_object,"Turn Left-",result))
        degree = desired_degree  
    elif desired_degree > degree:
        msg="Turn Right-"+str(desired_degree-degree)
        result=send_action(msg)
        action_log.append((dt_object,"Turn Right-",result))
        degree = desired_degree 
    i=0
    while begin+i != end :
        x_dif=node_list[begin+i+1][0]-node_list[begin+i][0]
        y_dif=node_list[begin+i+1][1]-node_list[begin+i][1]
        desired=degreeDict[(x_dif,y_dif)]
        if desired != degree:
            break
        i += 1
    msg="Forward-"+str(round(math.sqrt(x_diff**2+y_diff**2)*i,1))
    result=send_action(msg)
    if result != "s":
        parts=result.split("-")
        dist=parts[1]
        if abs(x_diff)+abs(y_diff)==2:
            steps=int(float(dist)/1.4)
        else:
            steps=int(dist)
        cur_x=pathDictionary['x'][-1]
        cur_y=pathDictionary['y'][-1]
        new_item = Items(id=parts[0], x_location = cur_x+(steps+1)*x_diff, y_location = cur_y+(steps+1)*y_diff)
        print(cur_x,cur_y,x_diff,y_diff,steps)
        for x in range(int(steps+2)):
            search_list[cur_y+y_diff*x][cur_x+x_diff*x] = 1
        pathDictionary['x'].append(cur_x+steps*x_diff)
        pathDictionary['y'].append(cur_y+steps*y_diff)
        db.session.add(new_item)
        db.session.commit()
        return search_list , 0 , True
    else:
        action_log.append((dt_object,"Forward-",result))
        if abs(x_diff)+abs(y_diff)==2:
            steps=round(math.sqrt(x_diff**2+y_diff**2)*i,1)/1.4
        else:
            steps=round(math.sqrt(x_diff**2+y_diff**2)*i,1)
        cur_x=pathDictionary['x'][-1]
        cur_y=pathDictionary['y'][-1]
        print(cur_x,cur_y,x_diff,y_diff,steps)
        for x in range(int(steps+1)):
            search_list[cur_y+y_diff*x][cur_x+x_diff*x] = 1
        pathDictionary['x'].append(node_list[begin+i][0])
        pathDictionary['y'].append(node_list[begin+i][1])
        return search_list , begin + i , False

@views.route('/', methods=['GET','POST'])
def home():
    dataDictionary={'x': [],'y': [],'text':[]}
    if request.form.get('action') != None:
        if request.form.get('action') == 'Clear Logs':
            action_log.clear()
        elif request.form.get('action') == 'Connect/Disconnect':
            action_log.append((dt_object,request.form.get('action'),''))
            global connected
            if not connected:
                try:
                    ws.connect(ip, timeout=100)
                    print("Connected to WebSocket server")
                    ws.send("web terminal")
                    result = ws.recv()
                    print(result)
                    connected=True
                except:
                    flash('Connection error', category='error')
            else:
                ws.close()
                connected=False
        elif request.form.get('action') == 'Update':
            if connected:
                results=update()
                new_item = Items(id=results[0], x_location = int(results[1]), y_location = int(results[2]))
                db.session.add(new_item)
                db.session.commit()
            else:
                flash('Rover is not connected', category='error')
        elif request.form.get('action') == 'Clear Database':
            Items.query.delete()
            db.session.commit()
        elif request.form.get('action') == 'Clear Path':
            global pathDictionary
            pathDictionary={'x': [0],'y': [0]}
        elif request.form.get('action') == 'Search Area':
            if connected:
                area_length=int(request.form.get('length'))
                area_width=int(int(request.form.get('width')))
                search_list=[]
                for x in range(area_width+1):
                    search_list.append([0]*(area_length+1))
                search_list[0][0]=1
                while True:
                    skip=False
                    print(search_list)
                    goal_cord=choose_longest_path(search_list)
                    print(goal_cord)
                    if goal_cord==None:
                        break
                    else:
                        node_list=[]
                        obstacles_list=[]
                        start=(pathDictionary['x'][-1],pathDictionary['y'][-1])
                        all_items=Items.query.all()
                        for obstacles_item in all_items:
                            obstacles_list.append((int(obstacles_item.x_location),int(obstacles_item.y_location)))
                        try:
                            path=path_finder(start,goal_cord,obstacles_list,area_length+1,area_width+1)
                        except:
                            search_list[goal_cord[1]][goal_cord[0]] = 1
                            skip = True
                        if skip != True:
                            node=path[goal_cord]
                            node_list.append(goal_cord)
                            while node != None:
                                node_list.append(node)
                                node=path[node]
                            node_list.reverse()
                            begin = 0
                            end = len(node_list)-1
                            obstruction=None
                            while begin != end and obstruction != True :
                                search_list, begin, obstruction=path_commands_v2(begin,end,node_list,search_list)
                        time.sleep(5)
            else:
                flash('Rover is not connected!', category='error')
        elif request.form.get('action') == 'Move to Item' or request.form.get('action') == 'Move to Point' :
            if connected:     
                node_list=[]
                obstacles_list=[]
                item=Items.query.filter_by(id=request.form.get('item')).first()
                start=(pathDictionary['x'][-1],pathDictionary['y'][-1])
                if request.form.get('action') == 'Move to Item':
                    goal=(int(item.x_location),int(item.y_location))
                else:
                    if request.form.get('x_cordinate') != "" and request.form.get('y_cordinate') != "":
                        goal=(int(request.form.get('x_cordinate')),int(request.form.get('y_cordinate')))
                    else:
                        flash('No value provided', category='error')
                all_items=Items.query.all()
                for obstacles_item in all_items:
                    if request.form.get('action') == 'Move to Item':
                        if obstacles_item.id == item.id or int(obstacles_item.x_location) > int(item.x_location) or int(obstacles_item.y_location) > int(item.y_location) :
                            continue
                    else:
                        if int(obstacles_item.x_location) > int(request.form.get('x_cordinate')) or int(obstacles_item.y_location) > int(request.form.get('y_cordinate')) :
                            continue
                    obstacles_list.append((int(obstacles_item.x_location),int(obstacles_item.y_location)))
                try:
                    path=path_finder(start,goal,obstacles_list,goal[0]+1,goal[1]+1)
                    node=path[goal]
                    node_list.append(goal)
                    while node != None:
                        node_list.append(node)
                        node=path[node]
                    node_list.reverse()
                    begin = 0
                    end = len(node_list)-1
                    while begin != end :
                        begin=path_commands(begin,end,node_list)
                except:
                    flash('Path is not available!', category='error')
            else:
                flash('Rover is not connected!', category='error')
        else:
            if connected:
                global degree
                if request.form.get('distance_degree') != "":
                    msg=request.form.get('action')+"-"+request.form.get('distance_degree')
                    print(msg)
                    result=send_action(msg)
                    if result=="Success":
                        if request.form.get('action') == "Forward":
                            calculate_pos(int(request.form.get('distance_degree')))
                        elif request.form.get('action') == "Backward":
                            calculate_pos(-int(request.form.get('distance_degree')))
                        elif request.form.get('action') == "Turn Right":
                            degree += int(request.form.get('distance_degree'))
                        else:
                            degree -= int(request.form.get('distance_degree'))
                    if degree >= 360:
                        degree -= 360
                    action_log.append((dt_object,request.form.get('action'),result))
                else:
                   flash('No value provided', category='error') 
            else:
                flash('Rover is not connected!', category='error')
    all_items=Items.query.all()
    for item in all_items:
        dataDictionary['text'].append(item.id)
        dataDictionary['x'].append(item.x_location)
        dataDictionary['y'].append(item.y_location)
    return render_template("home.html", log=action_log, items=all_items,battery=percentage,speed=speed, data=dataDictionary, connected=connected, path=pathDictionary)
