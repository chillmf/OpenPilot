defineReplace(cleanPath) {
    win32:1 ~= s|\\\\|/|g
    contains(1, ^/.*):pfx = /
    else:pfx =
    segs = $$split(1, /)
    out =
    for(seg, segs) {
        equals(seg, ..):out = $$member(out, 0, -2)
        else:!equals(seg, .):out += $$seg
    }
    return($$join(out, /, $$pfx))
}

defineReplace(targetPath) {
    return($$replace(1, /, $$QMAKE_DIR_SEP))
}

# For use in custom compilers which just copy files
win32:i_flag = i
defineReplace(stripSrcDir) {
    win32 {
        !contains(1, ^.:.*):1 = $$OUT_PWD/$$1
    } else {
        !contains(1, ^/.*):1 = $$OUT_PWD/$$1
    }
    out = $$cleanPath($$1)
    out ~= s|^$$re_escape($$PWD/)||$$i_flag
    return($$out)
}

isEmpty(TEST):CONFIG(debug, debug|release) {
    !debug_and_release|build_pass {
        TEST = 1
    }
}

isEmpty(GCS_LIBRARY_BASENAME) {
    GCS_LIBRARY_BASENAME = lib
}

DEFINES += GCS_LIBRARY_BASENAME=\\\"$$GCS_LIBRARY_BASENAME\\\"

equals(TEST, 1) {
    QT +=testlib
    DEFINES += WITH_TESTS
}

GCS_SOURCE_TREE = $$PWD
isEmpty(GCS_BUILD_TREE) {
    sub_dir = $$_PRO_FILE_PWD_
    sub_dir ~= s,^$$re_escape($$PWD),,
    GCS_BUILD_TREE = $$cleanPath($$OUT_PWD)
    GCS_BUILD_TREE ~= s,$$re_escape($$sub_dir)$,,
}
IDE_APP_PATH = $$GCS_BUILD_TREE/bin
macx {
    IDE_APP_TARGET   = "OpenPilot GCS"
    IDE_LIBRARY_PATH = $$IDE_APP_PATH/$${IDE_APP_TARGET}.app/Contents/PlugIns
    IDE_PLUGIN_PATH  = $$IDE_LIBRARY_PATH
    IDE_LIBEXEC_PATH = $$IDE_APP_PATH/$${IDE_APP_TARGET}.app/Contents/Resources
    IDE_DATA_PATH    = $$IDE_APP_PATH/$${IDE_APP_TARGET}.app/Contents/Resources
    IDE_DOC_PATH     = $$IDE_DATA_PATH/doc
    contains(QT_CONFIG, ppc):CONFIG += ppc x86
    copydata = 1
} else {
    win32 {
        contains(TEMPLATE, vc.*)|contains(TEMPLATE_PREFIX, vc):vcproj = 1
        IDE_APP_TARGET   = openpilotgcs
    } else {
        IDE_APP_WRAPPER  = openpilotgcs
        IDE_APP_TARGET   = openpilotgcs.bin
    }
    IDE_LIBRARY_PATH = $$GCS_BUILD_TREE/$$GCS_LIBRARY_BASENAME/openpilotgcs
    IDE_PLUGIN_PATH  = $$IDE_LIBRARY_PATH/plugins
    IDE_LIBEXEC_PATH = $$IDE_APP_PATH # FIXME
    IDE_DATA_PATH    = $$GCS_BUILD_TREE/share/openpilotgcs
    IDE_DOC_PATH     = $$GCS_BUILD_TREE/share/doc/openpilotgcs
    !isEqual(GCS_SOURCE_TREE, $$GCS_BUILD_TREE):copydata = 1
}

INCLUDEPATH += \
    $$GCS_SOURCE_TREE/src/libs

DEPENDPATH += \
    $$GCS_SOURCE_TREE/src/libs

LIBS += -L$$IDE_LIBRARY_PATH

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
#DEFINES += QT_USE_FAST_OPERATOR_PLUS
#DEFINES += QT_USE_FAST_CONCATENATION

unix {
    CONFIG(debug, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    CONFIG(release, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    CONFIG(debug, debug|release):MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    CONFIG(release, debug|release):MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc
    UI_DIR = $${OUT_PWD}/.uic
}

linux-g++-* {
    # Bail out on non-selfcontained libraries. Just a security measure
    # to prevent checking in code that does not compile on other platforms.
    QMAKE_LFLAGS += -Wl,--allow-shlib-undefined -Wl,--no-undefined
}

