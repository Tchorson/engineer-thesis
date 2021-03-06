const MongoClient = require('mongodb').MongoClient;
const express = require('express');
const app = express();
var path = require("path");
const uri = 'mongodb+srv://mtchorek:admin@information-collections-mz7lu.mongodb.net/google-drive?retryWrites=true&w=majority'
var clientCoordinates = "";
var clientMongo = null;
var routesCollection = null;
var usersCollection = null;
var currentUser = null;
var routePointsArray = [];
var bodyParser = require('body-parser');
app.use(bodyParser.json());

function init() {
    MongoClient.connect(uri, function (err, client) {
        if (err)
            console.log('Error occurred while connecting to MongoDB Atlas...\n', err);
        clientMongo = client;
        routesCollection = client.db("google-drive").collection("routes");
        usersCollection = client.db("google-drive").collection("users");
    });
}

// /location?id=502261249&latlang=50.0331315,20.123567
app.get('/location', (req, res) => { //send arduino coordinates
    console.log("Received arduino coordinates: " + req.query.latlang + " user: " + req.query.id);
    currentUser = req.query.id * 1; //String to int conversion
    clientCoordinates = req.query.latlang;
    usersCollection.updateOne(
        {"number": currentUser},
        {$set: {"lastPos": clientCoordinates}}
    ).then(answer => {
        res.type('text').send(answer.result.n > 0)});
});

//HTML purpose only
app.get('/check', (req, res) => { // send coordinates
    //User calls first, then sends his coordinates to database in order to prevent mixing routes between users
    console.log("sending coorinates to server: " + clientCoordinates+" from user: "+currentUser);
    res.send({clientCoordinates: clientCoordinates, id:currentUser});
});

//HTML purpose only
app.post('/respond', (req, res) => { // receive route
    console.log("/respond post request for user: "+req.body.id);
    res.send(extractJsonToCoordinatesArray(req.body.route, req.body.id*1));
});

// /del?id=502261249&latlang=50.0331315,20.123567
app.get('/del', (req, res) => { //remove single coordinate when user reach destination point
    var textCoordinatesData = 17;// Latitude, longitude and separator should be 17 characters length
	var coordinateLength = 8; //each of coordinates has 8 characters, including dot
	if(req.query.latlang.length <textCoordinatesData){{//There may be a situation when zero, the last decimal number is missing and there is a need to add it again in the missing place 
        var localCoordinatesArray  = req.query.latlang.split(",");
        console.log(localCoordinatesArray [0].length);
        if (localCoordinatesArray [0].length<coordinateLength){
            for(let size = localCoordinatesArray [0].length; size<coordinateLength;size++){
                localCoordinatesArray [0]+="0";
            }
        }
        if (localCoordinatesArray [1].length<coordinateLength){
            for(let size = localCoordinatesArray [1].length; size<coordinateLength;size++){
                localCoordinatesArray [1]+="0";
            }
        }
        var latlang = localCoordinatesArray [0]+","+localCoordinatesArray [1];
    }else{
        var latlang = req.query.latlang;
    }

    routesCollection.update({"number": req.query.id * 1},
        {
            "$pull": {"route": latlang}
        }).then(answer => {//String to int conversion
            if (answer.result.n*1 > 0)    console.log("Removed coordinates: " + req.query.latlang + " for user: " + req.query.id);
            else console.log("Coordinates: " + req.query.latlang + " for user: " + req.query.id+" has not been removed");
            res.type('text').send(answer.result.n > 0)});
});

app.get('/close', (req, res) => { //for debugging purpose only
    clientMongo.close();
});

// /route?id=502261249
app.get('/route', (req, res) => { //checks if route is calibrated, in case arduino has turned off
    currentUser = req.query.id * 1;
    routesCollection.count({"number": currentUser, "route": {"$exists": true}}).then(docsAmount => {
        if (docsAmount > 0) {
             routesCollection.find({"number": currentUser}).next().then(routeArray => {
                routePointsArray = routeArray.route;
                for (i = 0; i < routeArray.route.length; i++){
                    let stringCoordinates = routePointsArray.shift().split(",");
                    if (stringCoordinates[0] * 1.0 !== 0.0 || stringCoordinates[1] * 1.0 !== 0.0){//String to float conversion
                        console.log("Route has been already created for: " + currentUser);
                        res.type('text').send(true);
                        break
                    }
                }res.type('text').send(false);
            }
        );}
        else {
            console.log("Route is not created for user: " + currentUser);
            res.type('text').send(false);
        }
    })
});

// /get?id=502261249
app.get('/get', (req, res) => { //send to arduino 2d coordinates array
    currentUser = req.query.id * 1;
    var routePointsArray = null;
    routesCollection.find({"number": currentUser}).next().then(routeArray => {
        routePointsArray = routeArray.route;
        var coordinatesToSend = [];
		var maxAmountOfCoordinates = 10; //The optimal amount of coordinates that Arduino can keep during user navigation
        var amountOfCoordinates = routePointsArray.length > maxAmountOfCoordinates ? maxAmountOfCoordinates : routePointsArray.length;
        var restOfCoordinates = maxAmountOfCoordinates - amountOfCoordinates;

        for (i = 0; i < amountOfCoordinates; i++) {
            let stringCoordinates = routePointsArray.shift().split(",");
            console.log(stringCoordinates);
            coordinatesToSend.push([stringCoordinates[0] * 1.0, stringCoordinates[1] * 1.0]);//String to float conversion
        }
        for (i = 0; i < restOfCoordinates; i++) {
            coordinatesToSend.push([0.0, 0.0]);
        }
        res.type('text').send(coordinatesToSend);
    });
});

function extractJsonToCoordinatesArray(routeArray,user) { // convert json to object array
    var coordinatesArray = [];
    coordinatesArray.push(routeArray[0].start_location.lat.toFixed(5) + "," + routeArray[0].start_location.lng.toFixed(5));
    for (let i = 0; i < routeArray.length; i++)
        coordinatesArray.push(routeArray[i].end_location.lat.toFixed(5) + "," + routeArray[i].end_location.lng.toFixed(5))
    routePointsArray = coordinatesArray;
    console.log("Setting route for user: " + currentUser + " \n" + coordinatesArray);
    routesCollection.updateOne(
        {"number": user*1},
        {$set: {"route": coordinatesArray}}
        ).then(() => {return true});
}

app.get('/', function (req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
});

const PORT = process.env.PORT || 80;
app.listen(PORT, () => {
    console.log(`Server listening on port ${PORT}...`);
    init();
});