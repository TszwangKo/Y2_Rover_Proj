from flask import Flask
from flask_sqlalchemy import SQLAlchemy
from os import path

db = SQLAlchemy()
DB_NAME = "database.db"

def create_app():
    app=Flask(__name__)
    app.config['SECRET_KEY'] = '4_4ecrete_key'
    app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///{DB_NAME}'
    db.init_app(app)

    from .view import views
    from .add_items import add_items

    app.register_blueprint(views, url_prefix='/')
    app.register_blueprint(add_items, url_prefix='/')

    from .models import Items

    create_database(app)

    return app

def create_database(app):
    if not path.exists('website/' + DB_NAME):
        db.create_all(app=app)
        print('Created Database!')