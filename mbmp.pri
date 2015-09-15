documentation.path = $$MBMP_DOC_PATH/man1
documentation.files = ./misc/manpage/mbmp.1.gz
documentation.CONFIG = no_check_exist
documentation.extra = gzip --force --keep ./misc/manpage/mbmp.1
INSTALLS += documentation
