@rem Run.bat for NervGearVideo
@if "%1" NEQ "debug" (
    @if "%1" NEQ "release" (
	    @if "%1" NEQ "clean" (
            @echo The first parameter to run.bat must be one of debug, release or clean.
            @goto:EOF
		)
	)
)
@call ..\build.cmd me.takashiro.nervgearvideo bin/NervGearVideo-debug.apk me.takashiro.nervgearvideo.MainActivity %1 %2
