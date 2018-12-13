var sheet = require('./ss.json');

var s = require('fs').createWriteStream('ss.csv');
s.once('open', () => {
    for (var key in sheet)
        s.write(`${key.substr(2)}.png,${sheet[key].frame.x},${sheet[key].frame.y},${sheet[key].frame.width},${sheet[key].frame.height}\n`);
    s.end();
});
