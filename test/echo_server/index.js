var port = 3000;

var io = require('socket.io').listen(port);
console.log("Listening on port " + port);

/* Socket.IO events */
io.on("connection", function(socket){
    console.log("new connection");
    socket.on('test_text',function()
    {
        console.log("test text event received.");
    });

    socket.on('test_binary',function()
    {
       var args =Array.prototype.slice.call(arguments);
      if('buffer' == typeof args[0])
      {
        console.log("test binary event received,binary length:"+ args[0].length);
      }
    });

    socket.on('test ack',function()
    {
       var args =Array.prototype.slice.call(arguments);
      if('object' == typeof args[0])
      {
        console.log("test combo received,object:");
        console.log(args[0]+"")//log object
      }
      if(args.length>1 && 'function' == typeof args[args.length - 1])
      {
        console.log('need ack for test combo');
        var fn = args[args.length - 1];
        fn('ack response');//invoke ack callback function.
      }
    });

  });