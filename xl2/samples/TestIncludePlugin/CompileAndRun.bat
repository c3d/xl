copy ..\..\xl.syntax .

@rem ..\..\native\xl3 -bootstrap TestIncludePlugin.xl > TestIncludePlugin_native.cpp
@rem cl /EHsc /I..\..  TestIncludePlugin_native.cpp
@rem TestIncludePlugin_native

..\..\native\xl3 TestIncludePlugin.xl > TestIncludePlugin_native.cpp


