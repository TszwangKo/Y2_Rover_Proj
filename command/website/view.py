from flask import Blueprint, render_template, request, flash
from .implementation import path_finder
from datetime import datetime
from .models import Items
from . import db
import websocket
import math
import time

views = Blueprint('views',__name__)

ip="ws://14.192.246.189/"
ws = websocket.WebSocket()

# Global State Variable
current_state="Idle"
action_log=[]
percentage=50
speed=1024
connected=False
degree=0
pathDictionary={'x': [0],'y': [0]}
dt_object = datetime.fromtimestamp(datetime.timestamp(datetime.now())).strftime("%m/%d/%Y, %H:%M:%S")
searched_area_length=0
searched_area_width=0
degreeDict={(0, +1):0, (+1, +1):45, (+1, 0):90, (+1, -1):135, (0, -1):180, (-1, -1):225, (-1, 0):270, (-1, +1):315} 


@views.route('/', methods=['GET','POST'])
def home():

    dataDictionary={'x': [],'y': [],'text':[]}

    # return the battery percentage level 
    def lookupDict(OCV1,OCV2):

        SOC1=SOC2=None

        OCV_dict=[(3360,100),(3324,90),(3320,80),(3308,70),(3292,60),(3288,50),(3284,40),(3276,30),(3252,20),(3217,10)]

        for i in OCV_dict:
            if i[0]<=OCV1:
                SOC1=i[1]
            if i[0]<=OCV2:
                SOC2=i[1]
        
        if SOC1 is None:
            SOC1=0

        if SOC2 is None:
            SOC2=0
        
        total_SOC = SOC1/2 + SOC2/2
        return total_SOC

    # return the connection status and flash error if not connected
    def check_connection():
        global connected
        if not connected:
            flash('Rover is not connected!', category='error')
            return False
        return True

    # send and return the msg received from the Rover
    def send_action(msg):
        ws.send(msg)
        result=ws.recv()
        print("Received: " + result)
        return result

    # calculate the position of the Mars Rover
    def calculate_pos(mag):
        global degree
        cur_x=pathDictionary['x'][-1]
        cur_y=pathDictionary['y'][-1]
        next_x=round(cur_x+ mag * math.sin(degree/180*math.pi),1)
        next_y=round(cur_y+ mag * math.cos(degree/180*math.pi),1)
        pathDictionary['x'].append(next_x)
        pathDictionary['y'].append(next_y)

    # send command to Mars Rover to turn to suitable orientation
    def turned_to(desired_degree):
        global degree
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

    # Find the furthest cord from current cord. If all cord is searched, return None
    def choose_longest_path(search_list):
        global pathDictionary
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

    # Performing surrounding check using Mars Rover camera
    def surrounding_check(area_length,area_width,search_list):
        global pathDictionary
        cur_x=pathDictionary['x'][-1]
        cur_y=pathDictionary['y'][-1]
        for direction in degreeDict:
            detect_x=cur_x+direction[0]
            detect_y=cur_y+direction[1]
            if detect_x <= area_length and detect_y <= area_width and detect_x >= 0 and detect_y >= 0:
                if search_list[detect_y][detect_x]==0:
                    desired_degree=degreeDict[(direction[0],direction[1])]
                    turned_to(desired_degree)
                    result=send_action("Check")
                    if result != "s":
                        print(result)
                        text=input("Enter the name:")
                        new_item = Items(id=text, x_location = detect_x, y_location = detect_y)
                        db.session.add(new_item)
                        db.session.commit()
                    search_list[detect_y][detect_x]=1

    # Commands send to require update from Mars Rover
    def update():
        ws.send("Update")
        result = ws.recv()
        print("Received: " + result)
        parts = result.split("-")
        return parts

    # path commands algorithm assume there is no obstacles
    def path_commands(node_list):
        global pathDictionary
        i=0
        while i < len(node_list) :

            k=1

            #calculate the orientation of the first command
            x_diff=node_list[i+k][0]-node_list[i][0]
            y_diff=node_list[i+k][1]-node_list[i][1]
            desired_degree=degreeDict[(x_diff,y_diff)]
            turned_to(desired_degree)

            #combine the command if they have same orientation
            while i + k - 1 < len(node_list):
                k += 1 
                x_check=node_list[i+k][0]-node_list[i+k-1][0]
                y_check=node_list[i+k][1]-node_list[i+k-1][1]
                if x_check != x_diff and y_check != y_diff:
                    break
            
            #send the combined forward msg
            msg="1024S"+str(round(math.sqrt(x_diff**2+y_diff**2),1)*(k-1))+"FD"
            ws.send(msg)
            action_log.append((dt_object,"Forward-"))
            pathDictionary['x'].append(node_list[i+k][0])
            pathDictionary['y'].append(node_list[i+k][1])

            i += k-1

    # modified path commands algorithm to detect obstacles (minimum steps)
    def path_commands_v2(node_list,search_list):
        global pathDictionary
        x_diff=node_list[1][0]-node_list[0][0]
        y_diff=node_list[1][1]-node_list[0][1]
        desired_degree=degreeDict[(x_diff,y_diff)]
        turned_to(desired_degree)
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

    # Changing Rover status
    if not request.form.get('state'):
        global percentage
        global current_state
        next_state=request.form.get('state')
        if check_connection():
            if next_state == "Charging":
                result=send_action("Charging")
                percentage=None
                current_state="Charging"
                if result=="fully charged":
                    current_state="Idle"
                    result=send_action("Idle")
                    parts=result.split("-")
                    percentage=lookupDict(int(parts[0]),int(parts[1]))
                    flash('Cells fully charged and balanced!', category='success')
            elif next_state == "Idle":
                current_state="Idle"
                result=send_action("Idle")
                parts=result.split("-")
                percentage=lookupDict(int(parts[0]),int(parts[1]))
            else:
                current_state="Rover_on"
                result=send_action("Rover_on")
                percentage += float(result)/ (2* 1908000)

    # Handling different request from the command panel
    if not request.form.get('action'):
        global degree
        global connected
        global searched_area_length
        global searched_area_width
        global pathDictionary
        # Clear Log Function
        if request.form.get('action') == 'Clear Logs':
            action_log.clear()

        # Connect Function
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
        
        # Update Function
        elif request.form.get('action') == 'Update':
            if check_connection():
                results=update()
                new_item = Items(id=results[0], x_location = int(results[1]), y_location = int(results[2]))
                db.session.add(new_item)
                db.session.commit()

        # Clear Database Function
        elif request.form.get('action') == 'Clear Database':
            Items.query.delete()
            db.session.commit()

        # Clear Path Function
        elif request.form.get('action') == 'Clear Path':
            pathDictionary={'x': [0],'y': [0]}

        # Search Area Function
        elif request.form.get('action') == 'Search Area':
            if check_connection():
                area_length=int(request.form.get('length'))
                area_width=int(request.form.get('width'))
                #Check whether area already searched
                if searched_area_width >= area_width and searched_area_length >= area_length:
                    flash('Area already searched', category='error')
                else:
                    
                    #Update searched area
                    searched_area_width = max(searched_area_width, area_width)
                    searched_area_length = max(searched_area_length, area_length)
                    search_list=[[0]*(area_length+1)]* (area_width+1)
                    search_list[0][0]=1
                    goal_cord=choose_longest_path(search_list)
                    
                    #while there exist a unsearched coord
                    while goal_cord: 
                        print(goal_cord)

                        node_list=[]
                        obstacles_list=[]
                        start=(pathDictionary['x'][-1],pathDictionary['y'][-1])
                        all_items=Items.query.all()

                        #Load all known obstacles into calculation
                        for obstacles_item in all_items:
                            obstacles_list.append((int(obstacles_item.x_location),int(obstacles_item.y_location)))
                        
                        #Path Calculation
                        try:
                            path=path_finder(start,goal_cord,obstacles_list,area_length+1,area_width+1,search_list)
                        except:
                            search_list[goal_cord[1]][goal_cord[0]] = 1
                        else:
                            node=path[goal_cord]
                            node_list.append(goal_cord)
                            while node != None:
                                node_list.append(node)
                                node=path[node]
                            node_list.reverse()
                            path_commands_v2(node_list,search_list)
                            surrounding_check(area_length,area_width,search_list)
                        
                        goal_cord=choose_longest_path(search_list)
        
        # Move to Item / Move to Point Function
        elif request.form.get('action') == 'Move to Item' or request.form.get('action') == 'Move to Point' :
            if check_connection():
                #Check whether the area is searched before
                if searched_area_length == 0 or searched_area_width == 0 or searched_area_width < int(request.form.get('y_cordinate')) or searched_area_length < int(request.form.get('x_cordinate')):
                    flash("Area is not searched", category='error')
                else:
                    node_list=[]
                    obstacles_list=[]
                    item=Items.query.filter_by(id=request.form.get('item')).first()
                    start=(pathDictionary['x'][-1],pathDictionary['y'][-1])

                    #Set Goal coord as the x and y coord of the Item
                    if request.form.get('action') == 'Move to Item':
                        goal=(int(item.x_location),int(item.y_location))
                    #Check whether the goal coord provided for the Move to Point Command
                    else:
                        if not request.form.get('x_cordinate') and not request.form.get('y_cordinate'):
                            goal=(int(request.form.get('x_cordinate')),int(request.form.get('y_cordinate')))
                        else:
                            flash('No value provided', category='error')

                    #Adding known obstacles into path calculation
                    all_items=Items.query.all()
                    for obstacles_item in all_items:
                        if int(obstacles_item.x_location) > searched_area_length or int(obstacles_item.y_location) > searched_area_width :
                            continue
                        elif request.form.get('action') == 'Move to Item' and obstacles_item.id == item.id:
                            continue
                        obstacles_list.append((int(obstacles_item.x_location),int(obstacles_item.y_location)))
                    
                    #Path calculation
                    try:
                        path=path_finder(start,goal,obstacles_list,searched_area_length+1,searched_area_width+1,[])
                    except:
                        flash('Path is not available!', category='error')
                    else:
                        node=path[goal]
                        node_list.append(goal)
                        while node != None:
                            node_list.append(node)
                            node=path[node]
                        node_list.reverse()
                        path_commands(node_list)
        
        # Forward / Backward / Turn Left / Right Function
        else:
            if check_connection():
                if not request.form.get('distance_degree'):
                    # Forward
                    if request.form.get('action') == "Forward":
                        msg="1024S"+request.form.get('distance_degree')+"FD"
                        calculate_pos(int(request.form.get('distance_degree')))
                    # Backward
                    elif request.form.get('action') == "Backward":
                        msg="1024S"+request.form.get('distance_degree')+"BD"
                        calculate_pos(-int(request.form.get('distance_degree')))
                    #Turn Right
                    elif request.form.get('action') == "Turn Right":
                        msg="300S"+request.form.get('distance_degree')+"RD"
                        degree += int(request.form.get('distance_degree'))
                    #Turn Left
                    else:
                        msg="300S"+request.form.get('distance_degree')+"LD"
                        degree -= int(request.form.get('distance_degree'))
                    print(msg)
                    ws.send(msg)
                    degree %= 360
                    action_log.append((dt_object,request.form.get('action')))
                else:
                   flash('No value provided', category='error') 

    # Query the Database to display on the control panel
    all_items=Items.query.all()
    for item in all_items:
        dataDictionary['text'].append(item.id)
        dataDictionary['x'].append(item.x_location)
        dataDictionary['y'].append(item.y_location)
    
    #render the html page
    return render_template("home.html", log=action_log, items=all_items,battery=percentage,speed=speed, data=dataDictionary, connected=connected, path=pathDictionary, current_state=current_state)
