from website import create_app

"""
to do:
primary key should be x and y location
(weighted map/turning) increase efficiency
exchange info with control
code refactoring

"""

app=create_app()

if __name__=="__main__":
    app.run(debug=True)