set VarSollutionDirectory=%~dp0\build
pushd %~dp0
mkdir %VarSollutionDirectory%
cd %VarSollutionDirectory%
cmake -g "Visual Studio 12" ..
popd
pause