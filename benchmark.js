var Magic = require('.').Magic;

var magic = new Magic(),
        buf = new Buffer('import Options\nfrom os import unlink, symlink');
        
var timeStart = Date.now();
var count = 0;
function next(){ 
  magic.detect(buf, function(err, result) {
    if (err) throw err;
    count++;
    
    if(Date.now() - 10000 > timeStart){
        console.log("-- %d ops/sec " , count/10  );
    }else{
        next();
    }
    //console.log(result);
  });
}

next();
next();
next();
next();

//-- 999.2 ops/sec