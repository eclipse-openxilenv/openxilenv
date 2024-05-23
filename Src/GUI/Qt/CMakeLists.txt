target_sources(XilEnvGui PRIVATE
    BlackboardObserver.cpp
    ConfigFilter.cpp
    HotkeyHandler.cpp
    MainWindow.cpp
    Sheets.cpp
    ValueInput.cpp
    FileDialog.cpp
    IsAlreadyRunnung.cpp
    MainWinowSyncWithOtherThreads.cpp
    QtIniFile.cpp
    StatusBar.cpp
    WindowUpdateTimers.cpp
    Hotkey.cpp
    Main.cpp
    PhysValueInput.cpp
    QVariantConvert.cpp
    TextValueInput.cpp
    DarkMode.cpp
    ColorHelpers.cpp
    StringHelpers.cpp
# ui files
    MainWindow.ui
    
    Icons.qrc
)

add_subdirectory("Data")
add_subdirectory("Dialog")
add_subdirectory("Widgets")
