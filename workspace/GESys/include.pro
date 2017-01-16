CONFIG *= link_prl
DEPENDPATH += . ../GESys
INCLUDEPATH += ..
LIBS += -L../GESys -lGESys
PRE_TARGETDEPS += ../GESys/libGESys.a
