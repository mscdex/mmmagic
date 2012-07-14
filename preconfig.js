var fs = require('fs');

fs.createReadStream('./deps/libmagic/config.h.' + process.platform)
  .pipe(fs.createWriteStream('./deps/libmagic/config.h'));

fs.createReadStream('./deps/libmagic/pcre/config.h.' + process.platform)
  .pipe(fs.createWriteStream('./deps/libmagic/pcre/config.h'));