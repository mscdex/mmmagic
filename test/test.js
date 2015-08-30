var mmm = require('../lib/index');

var path = require('path'),
    assert = require('assert'),
    fs = require('fs');

var t = -1,
    group = path.basename(__filename, '.js') + '/';

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
  if (t === tests.length - 1)
    return;
  var v = tests[++t];
  v.run.call(v);
}

function makeMsg(msg, what) {
  return '[' + group + (what || tests[t].what) + ']: ' + msg;
}

process.once('uncaughtException', function(err) {
  if (t > -1 && !/(?:^|\n)AssertionError: /i.test(''+err))
    console.error(makeMsg('Unexpected Exception:'));

  throw err;
}).once('exit', function() {
  assert(t === tests.length - 1,
         makeMsg('Only finished ' + (t + 1) + '/' + tests.length + ' tests',
                 '_exit'));
});

next();
