PLUGINOBJECTS = ofxsThreadSuite.o tinythread.o ofxsOGLTextRenderer.o ofxsOGLFontData.o src/P_Shake.o
PLUGINNAME = P_Shake

SRC_DIR := src

OFXPATH ?= openfx
OFXSEXTPATH ?= support

PATHTOROOT = $(OFXPATH)/Support

include Makefile.master

CXXFLAGS += -DOFX_EXTENSIONS_VEGAS -DOFX_EXTENSIONS_NUKE -DOFX_EXTENSIONS_NATRON -DOFX_EXTENSIONS_TUTTLE -DOFX_SUPPORTS_OPENGLRENDER

CXXFLAGS += -I./Misc -I$(OFXSEXTPATH)
VPATH += ./Misc $(OFXSEXTPATH)