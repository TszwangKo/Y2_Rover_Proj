from . import db

class Items(db.Model):
    id = db.Column(db.String(150),primary_key=True)
    x_location = db.Column(db.Integer)
    y_location = db.Column(db.Integer) 
