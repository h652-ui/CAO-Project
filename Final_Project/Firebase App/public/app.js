var databasePath = database.ref();
var URLReading, dateTimeReading, statusReading;
var table_html = "";
// Attach an asynchronous callback to read the data
databasePath.on('value', (snapshot) => {
    document.getElementById("h_table").innerHTML = "";
    table_html = "";
    table_html += "<tr><th>Image</th><th>Date & Time</th><th>Lock Status</th><th>Delete Button</th></tr>"
    snapshot.forEach((childSnapshot) => {
        URLReading = childSnapshot.child('url').val();
        dateTimeReading = childSnapshot.child('date').val();
        statusReading = childSnapshot.child('status').val();
        table_html += "<tr id='" + childSnapshot.key + "'><td><img src='" + URLReading + "'></td><td>" + dateTimeReading + "</td><td>"; 
        if(statusReading == "True")
        table_html += "Opened</td><td><a href='#' onclick = deleteNode('" + childSnapshot.key + "')>Delete</a></td></tr>"
        else
          table_html+= "Closed</td><td><a href='#' onclick = deleteNode('" + childSnapshot.key + "')>Delete</a></td></tr>";
    });
    document.getElementById('h_table').innerHTML += table_html;
}, (errorObject) => {
    console.log('The read failed: ' + errorObject.name);
});

function deleteNode(path) {
    document.getElementById(path).remove();
    document.getElementById("h_table").innerHTML = "";
    table_html = "";
    database.ref(path).remove();
}