var grid = require('./grid.json');

var f = grid.frames.map(x => x.frame);

grid.meta.layers[0].cels.forEach(x => f[x.frame].str = x.data);

var s = require('fs').createWriteStream('grid.csv');
s.once('open', () => {
    f.forEach(x => s.write(`${x.x},${x.y},${x.w},${x.h},${x.str || '0,0'}\n`));
    s.end();
});
