
Pod::Spec.new do |s|

    s.name          = "SocketIO-Client-CPP"
    s.version       = "1.6.1"
    s.summary       = "SocketIO-Client-CPP"
    s.homepage      = "https://github.com/socketio/socket.io-client-cpp"
    s.license       = {
    	:type => 'MIT',
    	:file => 'LICENSE'
    	}
    s.requires_arc  = false
    s.authors       = {
      "Melo Yao" => "melode11@gmail.com"
      }

    s.library = 'c++'
    s.pod_target_xcconfig = {
       'GCC_C_LANGUAGE_STANDARD' => 'gnu99',
       'CLANG_CXX_LANGUAGE_STANDARD' => 'gnu++0x',
       'CLANG_CXX_LIBRARY' => 'libc++'
    }

    s.source        = {
        :git => "https://github.com/socketio/socket.io-client-cpp.git",
        :tag => s.version.to_s,
        :submodules => true
        }

    s.dependency              'boost', '~> 1.57.0'

    # s.public_header_files   = 'lib/websocketpp/websocketpp/**/*.{hpp,h}',
    #                           'lib/rapidjson/include/rapidjson/**/*.{hpp,h}',
    #                           'src/*.{hpp,h}'
    #
    # s.private_header_files  = 'src/internal/*.{hpp,h}'
    #
    # s.source_files          = 'src/**/*.{cpp,c}'

    s.source_files =        'lib/websocketpp/websocketpp/**/*.{hpp,h}',
                            'lib/rapidjson/include/rapidjson/**/*.{hpp,h}',
                            'src/*.{hpp,h}',
                            'src/internal/*.{hpp,h}'

    s.public_header_files = 'src/*.{hpp,h}'

end
