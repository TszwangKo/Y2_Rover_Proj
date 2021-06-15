from flask import Blueprint, render_template, request, flash
from .implementation import path_finder
from datetime import datetime
from .models import Items
from . import db
import websocket
import math
import time

views = Blueprint('views',__name__)

current_state="Idle"
action_log=[]
percentage=50
speed=1024
connected=False
degree=0
random=0
pathDictionary={'x': [0],'y': [0]}
dt_object = datetime.fromtimestamp(datetime.timestamp(datetime.now())).strftime("%m/%d/%Y, %H:%M:%S")
searched_area_length=3
searched_area_width=5

#ip="ws://81.136.3.190/" Hubert
#ip= "ws://58.176.226.198:8765/"
ip="ws://14.192.246.189/"

ws = websocket.WebSocket()

def lookupDict(OCV1,OCV2):
    if OCV1 >= 3360 :
        SOC1 = 100
    elif 3324 <= OCV1 < 3360:
        SOC1 = 90
    elif 3320 <= OCV1 < 3324:
        SOC1 = 80
    elif 3308 <= OCV1 < 3320:
        SOC1 = 70
    elif 3292 <= OCV1 < 3308:
        SOC1 = 60
    elif 3288 <= OCV1 < 3292:
        SOC1 = 50
    elif 3284 <= OCV1 < 3288:
        SOC1 = 40
    elif 3276 <= OCV1 < 3284:
        SOC1 = 30
    elif 3252 <= OCV1 < 3276:
        SOC1 = 20
    elif 3217 <= OCV1 < 3252:
        SOC1 = 10
    elif OCV1 < 3217:
        SOC1 = 0
        
    if OCV2 >= 3360:
        SOC2 = 100
    elif 3324 <= OCV2 < 3360:
        SOC2 = 90;
    elif 3320 <= OCV2 < 3324:
        SOC2 = 80;
    elif 3308 <= OCV2 < 3320:
        SOC2 = 70;
    elif 3292 <= OCV2 < 3308:
        SOC2 = 60;
    elif 3288 <= OCV2 < 3292:
        SOC2 = 50;
    elif 3284 <= OCV2 < 3288:
        SOC2 = 40;
    elif 3276 <= OCV2 < 3284:
        SOC2 = 30;
    elif 3252 <= OCV2 < 3276:
        SOC2 = 20;
    elif 3217 <= OCV2 < 3252:
        SOC2 = 10;
    elif OCV2 < 3217:
        SOC2 = 0;
    
    total_SOC = SOC1/2 + SOC2/2
    return total_SOC

def calculate_pos(mag):
    cur_x=pathDictionary['x'][-1]
    cur_y=pathDictionary['y'][-1]
    next_x=round(cur_x+ mag * math.sin(degree/180*math.pi),1)
    next_y=round(cur_y+ mag * math.cos(degree/180*math.pi),1)
    pathDictionary['x'].append(next_x)
    pathDictionary['y'].append(next_y)

def send_action(msg):
    ws.send(msg)
    result=ws.recv()
    print("Received: " + result)
    return result

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

def surrounding_check(area_length,area_width,search_list):
    global degree
    global random
    degreeDict={(0, +1):0, (+1, +1):45, (+1, 0):90, (+1, -1):135, (0, -1):180, (-1, -1):225, (-1, 0):270, (-1, +1):315}
    cur_x=pathDictionary['x'][-1]
    cur_y=pathDictionary['y'][-1]
    for direction in degreeDict:
        detect_x=cur_x+direction[0]
        detect_y=cur_y+direction[1]
        if detect_x <= area_length and detect_y <= area_width and detect_x >= 0 and detect_y >= 0:
            if search_list[detect_y][detect_x]==0:
                desired_degree=degreeDict[(direction[0],direction[1])]
                if degree > desired_degree:
                    msg="300S"+str(degree-desired_degree)+"LD"
                    ws.send(msg)
                    action_log.append((dt_object,"Turn Left-"))
                    degree = desired_degree  
                elif desired_degree > degree:
                    msg="300S"+str(desired_degree-degree)+"RD"
                    ws.send(msg)
                    action_log.append((dt_object,"Turn Right-"))
                    degree = desired_degree
                result=send_action("Check")
                if result != "s":
                    print(result)
                    text=input("Enter the name:")
                    new_item = Items(id=text, x_location = detect_x, y_location = detect_y)
                    db.session.add(new_item)
                    db.session.commit()
                search_list[detect_y][detect_x]=1
    return search_list

def little_surrounding_check(area_length,area_width,search_list):
    degreeDict={0:(0, +1), 45:(+1, +1), 90:(+1, 0), 135:(+1, -1), 180:(0, -1), 225:(-1, -1), 270:(-1, 0), 315:(-1,+1)}
    cur_x=pathDictionary['x'][-1]
    cur_y=pathDictionary['y'][-1]
    direction=degreeDict[degree]
    detect_x=cur_x+direction[0]
    detect_y=cur_y+direction[1]
    if detect_x <= area_length and detect_y <= area_width and detect_x >= 0 and detect_y >= 0:
        if search_list[detect_y][detect_x]==0:
            result=send_action("Check")
            if result != "s":
                new_item = Items(id=result, x_location = detect_x, y_location = detect_y)
                db.session.add(new_item)
            search_list[detect_y][detect_x]=1
    return search_list

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
        msg=str(degree-desired_degree)+"LD"
        ws.send(msg)
        action_log.append((dt_object,"Turn Left-"))
        degree = desired_degree  
    elif desired_degree > degree:
        msg=str(desired_degree-degree)+"RD"
        ws.send(msg)
        action_log.append((dt_object,"Turn Right-"))
        degree = desired_degree 
    i=0
    while begin+i != end :
        x_dif=node_list[begin+i+1][0]-node_list[begin+i][0]
        y_dif=node_list[begin+i+1][1]-node_list[begin+i][1]
        desired=degreeDict[(x_dif,y_dif)]
        if desired != degree:
            break
        i += 1
    msg="1024S"+str(round(math.sqrt(x_diff**2+y_diff**2),1))+"FD"
    ws.send(msg)
    action_log.append((dt_object,"Forward-"))
    pathDictionary['x'].append(node_list[begin+i][0])
    pathDictionary['y'].append(node_list[begin+i][1])
    return begin + i

def path_commands_v2(node_list,search_list):
    global degree
    degreeDict={(+1, 0):90, (0, -1):180, (-1, 0):270, (0, +1):0, (-1, -1):225, (-1, +1):315, (+1, -1):135, (+1, +1):45}
    x_diff=node_list[1][0]-node_list[0][0]
    y_diff=node_list[1][1]-node_list[0][1]
    desired_degree=degreeDict[(x_diff,y_diff)]
    if degree > desired_degree:
        msg=str(degree-desired_degree)+"LD"
        print(msg)
        ws.send(msg)
        action_log.append((dt_object,"Turn Left-"))
        degree = desired_degree  
    elif desired_degree > degree:
        msg=str(desired_degree-degree)+"RD"
        print(msg)
        ws.send(msg)
        action_log.append((dt_object,"Turn Right-"))
        degree = desired_degree 
    time.sleep(2)
    if abs(x_diff) + abs(y_diff)== 2:
        msg="1024S140FD"
    else:
        msg="1024S100FD"
    print(msg)
    ws.send(msg)
    cur_x=pathDictionary['x'][-1]
    cur_y=pathDictionary['y'][-1]
    print(cur_x,cur_y,x_diff,y_diff)
    search_list[cur_y+y_diff][cur_x+x_diff] = 1
    pathDictionary['x'].append(cur_x+x_diff)
    pathDictionary['y'].append(cur_y+y_diff)
    return search_list

@views.route('/', methods=['GET','POST'])
def home():
    dataDictionary={'x': [],'y': [],'text':[]}
    if request.form.get('state') != None:
        next_state=request.form.get('state')
        global current_state
        global percentage
        global connected
        if next_state == "Charging":
            if connected:
                result=send_action("Charging")
                percentage=None
                current_state="Charging"
                if result=="fully charged":
                    current_state="Idle"
                    result=send_action("Idle")
                    parts=result.split("-")
                    percentage=lookupDict(int(parts[0]),int(parts[1]))
                    flash('Cells fully charged and balanced!', category='success')
            else:
                flash('Rover is not connected', category='error')
        elif next_state == "Idle":
            if connected:
                current_state="Idle"
                result=send_action("Idle")
                parts=result.split("-")
                percentage=lookupDict(int(parts[0]),int(parts[1]))
            else:
                flash('Rover is not connected', category='error')
        else:
            if connected:
                current_state="Rover_on"
                result=send_action("Rover_on")
                percentage += float(result)/ (2* 1908000)
            else:
                flash('Rover is not connected', category='error')
    if request.form.get('action') != None:
        if request.form.get('action') == 'Clear Logs':
            action_log.clear()
        elif request.form.get('action') == 'Connect/Disconnect':
            action_log.append((dt_object,request.form.get('action'),''))
            if not connected:
                try:
                    ws.connect(ip, timeout=100)
                    print("Connected to WebSocket server")
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
                global searched_area_length
                global searched_area_width
                area_length=int(request.form.get('length'))
                area_width=int(request.form.get('width'))
                if searched_area_width >= area_width and searched_area_length >= area_length:
                    flash('Area already searched', category='error')
                else:
                    if area_width > searched_area_width:
                        searched_area_width = area_width
                    if area_length > searched_area_length:
                        searched_area_length = area_length
                    search_list=[]
                    for x in range(area_width+1):
                        search_list.append([0]*(area_length+1))
                    search_list[0][0]=1
                    while True:
                        skip=False
                        # for z in search_list:
                        #     print(z)
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
                                path=path_finder(start,goal_cord,obstacles_list,area_length+1,area_width+1,search_list)
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
                                search_list=path_commands_v2(node_list,search_list)
                                search_list=little_surrounding_check(area_length,area_width,search_list)
                                #print(search_list)
            else:
                flash('Rover is not connected!', category='error')
        elif request.form.get('action') == 'Move to Item' or request.form.get('action') == 'Move to Point' :
            if connected:
                if searched_area_length == 0 or searched_area_width == 0 or searched_area_width < int(request.form.get('y_cordinate')) or searched_area_length < int(request.form.get('x_cordinate')):
                    flash("Area is not searched", category='error')
                else:
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
                            if int(obstacles_item.x_location) > searched_area_length or int(obstacles_item.y_location) > searched_area_width :
                                continue
                        obstacles_list.append((int(obstacles_item.x_location),int(obstacles_item.y_location)))
                    skip=False
                    try:
                        path=path_finder(start,goal,obstacles_list,searched_area_length+1,searched_area_width+1,[])
                    except:
                        flash('Path is not available!', category='error')
                        skip = True
                    node=path[goal]
                    node_list.append(goal)
                    if skip != True:
                        while node != None:
                            node_list.append(node)
                            node=path[node]
                        node_list.reverse()
                        begin = 0
                        end = len(node_list)-1
                        while begin != end :
                            begin=path_commands(begin,end,node_list)
            else:
                flash('Rover is not connected!', category='error')
        else:
            if connected:
                global degree
                if request.form.get('distance_degree') != "":
                    msg=""
                    if request.form.get('action') == "Forward":
                        msg="1024S"+request.form.get('distance_degree')+"FD"
                        calculate_pos(int(request.form.get('distance_degree')))
                    elif request.form.get('action') == "Backward":
                        msg="1024S"+request.form.get('distance_degree')+"BD"
                        calculate_pos(-int(request.form.get('distance_degree')))
                    elif request.form.get('action') == "Turn Right":
                        msg="300S"+request.form.get('distance_degree')+"RD"
                        degree += int(request.form.get('distance_degree'))
                    else:
                        msg="300S"+request.form.get('distance_degree')+"LD"
                        degree -= int(request.form.get('distance_degree'))
                    print(msg)
                    ws.send(msg)
                    if degree >= 360:
                        degree -= 360
                    action_log.append((dt_object,request.form.get('action')))
                else:
                   flash('No value provided', category='error') 
            else:
                flash('Rover is not connected!', category='error')
    all_items=Items.query.all()
    for item in all_items:
        dataDictionary['text'].append(item.id)
        dataDictionary['x'].append(item.x_location)
        dataDictionary['y'].append(item.y_location)
    return render_template("home.html", log=action_log, items=all_items,battery=percentage,speed=speed, data=dataDictionary, connected=connected, path=pathDictionary, current_state=current_state)
