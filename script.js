// Get current sensor readings when the page loads
window.addEventListener('load', getReadings);

// Create Temperature Chart
var chartT = new Highcharts.Chart({
  chart:{
    renderTo:'chart-temperature'
  },
  series: [
    {
      name: 'Temperatur Kuehlraum 1',
      type: 'line',
      color: '#101D42',
      marker: {
        symbol: 'circle',
        radius: 3,
        fillColor: '#101D42',
      }
    },
    {
      name: 'Temperatur Kuehlraum 2',
      type: 'line',
      color: '#00A6A6',
      marker: {
        symbol: 'square',
        radius: 3,
        fillColor: '#00A6A6',
      }
    },
    {
      name: 'Temperatur Kuehlraum 3',
      type: 'line',
      color: '#8B2635',
      marker: {
        symbol: 'triangle',
        radius: 3,
        fillColor: '#8B2635',
      }
    },
  ],
  title: {
    text: undefined
  },
  xAxis: {
    tickInterval: 1000,
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: {
      text: 'Temperatur in Grad Celsius'
    }
  },
  credits: {
    enabled: false
  }
});

  function plotTemperature(jsonValue) {
    var keys = Object.keys(jsonValue);
    for (var i = 0; i < keys.length; i++) {
      var values = jsonValue[i].values;
      for (var j = 0; j < values.length; j++) {
        var y = Number(values[j][0]);
        var timestamp = Number(values[j][1] * 1000);
        if (timestamp > 0){
          var x = new Date(timestamp).toISOString().substr(11, 8); // convert timestamp to human-readable format
          chartT.series[i].addPoint([x, y], true, false, true);
        }
      }
    }
  }
  
// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      plotTemperature(myObj);
    }
  };
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);

  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    plotTemperature(myObj);
  }, false);
}