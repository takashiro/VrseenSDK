@rem Run.bat for Vr360
@if "%1" NEQ "debug" (
    @if "%1" NEQ "release" (
	    @if "%1" NEQ "clean" (
            @echo The first parameter to run.bat must be one of debug, release or clean.
            @goto:EOF
		)
	)
)
call ..\build.cmd me.takashiro.nervgear.photo NervGearPhoto-debug.apk me.takashiro.nervgear.MainActivity %1 %2
