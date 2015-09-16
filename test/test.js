var mmm = require('../lib/index');

var path = require('path');
var assert = require('assert');
var fs = require('fs');
var format = require('util').format;

var t = -1;
var group = path.basename(__filename, '.js') + '/';
var timeout;

var tests = [
  { run: function() {
      var magic = new mmm.Magic(mmm.MAGIC_MIME_TYPE);
      magic.detectFile(path.join(__dirname, '..', 'src', 'binding.cc'),
                       function(err, result) {
        assert.strictEqual(err, null);
        assert.strictEqual(result, 'text/x-c++');
        next();
      });
    },
    what: 'detectFile - Normal operation, mime type'
  },
  { run: function() {
      var magic = new mmm.Magic(mmm.MAGIC_MIME_ENCODING);
      magic.detectFile(path.join(__dirname, '..', 'src', 'binding.cc'),
                       function(err, result) {
        assert.strictEqual(err, null);
        assert.strictEqual(result, 'us-ascii');
        next();
      });
    },
    what: 'detectFile - Normal operation, encoding'
  },
  { run: function() {
      var magic = new mmm.Magic(mmm.MAGIC_MIME);
      magic.detectFile(path.join(__dirname, '..', 'src', 'binding.cc'),
                       function(err, result) {
        assert.strictEqual(err, null);
        assert.strictEqual(result, 'text/x-c++; charset=us-ascii');
        next();
      });
    },
    what: 'detectFile - Normal operation, mime type + encoding'
  },
  { run: function() {
      var magic = new mmm.Magic();
      magic.detectFile(path.join(__dirname, '..', 'src', 'binding.cc'),
                       function(err, result) {
        assert.strictEqual(err, null);
        assert.strictEqual(/^C\+\+ source, ASCII text/.test(result), true);
        next();
      });
    },
    what: 'detectFile - Normal operation, description'
  },
  { run: function() {
      var magic = new mmm.Magic(mmm.MAGIC_MIME_TYPE | mmm.MAGIC_CONTINUE);
      magic.detectFile(path.join(__dirname, '..', 'src', 'binding.cc'),
                       function(err, result) {
        assert.strictEqual(err, null);
        assert.strictEqual(Array.isArray(result), true);
        assert.strictEqual(result[0], 'text/x-c++');
        next();
      });
    },
    what: 'detectFile - Normal operation, find all matches'
  },
  { run: function() {
      var magic = new mmm.Magic(mmm.MAGIC_MIME_TYPE);
      magic.detectFile('/no/such/path1234567', function(err, result) {
        assert(err);
        assert.strictEqual(result, undefined);
        next();
      });
    },
    what: 'detectFile - Nonexistent file'
  },
  { run: function() {
      var magic = new mmm.Magic(mmm.MAGIC_MIME_TYPE);
      magic.detectFile(path.join(__dirname, 'fixtures', 'tést.txt'),
                       function(err, result) {
        assert.strictEqual(err, null);
        assert.strictEqual(result, 'text/x-c++');
        next();
      });
    },
    what: 'detectFile - UTF-8 filename'
  },
  { run: function() {
      var buf = fs.readFileSync(path.join(__dirname, '..', 'src', 'binding.cc'));
      var magic = new mmm.Magic(mmm.MAGIC_MIME_TYPE);
      magic.detect(buf, function(err, result) {
        assert.strictEqual(err, null);
        assert.strictEqual(result, 'text/x-c++');
        next();
      });
    },
    what: 'detect - Normal operation, mime type'
  },
];

function next() {
  clearTimeout(timeout);
  if (t > -1)
    console.log('Finished  %j', tests[t].what)
  if (t === tests.length - 1)
    return;
  var v = tests[++t];
  timeout = setTimeout(function() {
    throw new Error(format('Test case %j timed out', v.what));
  }, 10 * 1000);
  console.log('Executing %j', v.what);
  v.run.call(v);
}

function makeMsg(msg) {
  var fmtargs = ['[%s]: ' + msg, tests[t].what];
  for (var i = 1; i < arguments.length; ++i)
    fmtargs.push(arguments[i]);
  return format.apply(null, fmtargs);
}

process.once('uncaughtException', function(err) {
  if (t > -1 && !/(?:^|\n)AssertionError: /i.test(''+err))
    console.error(makeMsg('Unexpected Exception:'));

  throw err;
}).once('exit', function() {
  assert(t === tests.length - 1,
         makeMsg('Only finished %d/%d tests', (t + 1), tests.length));
});

next();
