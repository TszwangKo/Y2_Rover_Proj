from website import create_app

#to do:
#response log
#speed?
#Battery percentage
#visual representation of the map
#map of what vision saw
#frontend prettier????


app=create_app()

if __name__=="__main__":
    app.run(debug=True)