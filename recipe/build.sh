cmake -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=$PREFIX $SRC -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_CXX_FLAGS_DEBUG=-fdebug-prefix-map=$SRC_DIR=/usr/local/src/conda/sqltoast-1.0.0
make install