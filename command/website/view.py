from flask import Blueprint, render_template, request, flash
from datetime import datetime
from .models import Items
from . import db
import websocket
# import socket

views = Blueprint('views',__name__)

action_log=[]
percentage=100
speed=1024

# def send_action(msg):
#     s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#     s.connect(('192.168.1.229', 1234))
#     s.send(msg.encode("utf-8"))

def send_action(msg):
    ws = websocket.WebSocket()
    ws.connect("ws://81.136.3.190/", timeout=3)
    print("Connected to WebSocket server")
    ws.send(msg)
    result = ws.recv()
    print("Received: " + result)
    ws.close()

def update():
    ws = websocket.WebSocket()
    ws.connect("ws://81.136.3.190/", timeout=3)
    print("Connected to WebSocket server")
    ws.send("Update")
    result = ws.recv()
    print("Received: " + result)
    parts = result.split("-")
    ws.close()
    return parts

@views.route('/', methods=['GET','POST'])
def home():
    dt_object = datetime.fromtimestamp(datetime.timestamp(datetime.now())).strftime("%m/%d/%Y, %H:%M:%S")
    if request.form.get('action') != None:
        action_log.append((dt_object,request.form.get('action')))
        if request.form.get('action') == 'Clear Logs':
            action_log.clear()
        elif request.form.get('action') == 'Update':
            try:
                results=update()
                new_item = Items(id=results[0], x_location = int(results[1]), y_location = int(results[2]))
                db.session.add(new_item)
                db.session.commit()
            except:
                flash('Connection error', category='error')
        else:
            try:
                send_action(request.form.get('action'))
            except:
                flash('Connection error', category='error')
    all_items=Items.query.all()
    return render_template("home.html", log=action_log, items=all_items,battery=percentage,speed=speed)