from flask import Blueprint, render_template, request, flash
from .models import Items
from . import db

add_items = Blueprint('add_items',__name__)

@add_items.route('/add_items', methods=['GET','POST'])
def add():
    if request.method =='POST':
        
        id = request.form.get('id')
        x_location = request.form.get('x_location')
        y_location = request.form.get('y_location')

        item = Items.query.filter_by(id=id).first()

        if item:
            flash('Item already exists.', category='error')
        else:
            new_item = Items(id=id, x_location=int(x_location), y_location=int(y_location))
            db.session.add(new_item)
            db.session.commit()
            flash('Item created!', category='success')

    return render_template("add_items.html")