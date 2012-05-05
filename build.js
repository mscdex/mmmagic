if (process.platform === 'win32')
  process.exit(0);
else {
  var child = require('child_process').spawn('node-waf', ['configure', 'build']);
  child.stdout.pipe(process.stdout);
  child.stderr.pipe(process.stderr);
  child.on('exit', function(code) {
    process.exit(code);
  });
}