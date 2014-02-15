set VarSollutionDirectory=%~dp0\build
pushd %~dp0
mkdir %VarSollutionDirectory%
cd %VarSollutionDirectory%
cmake ..
popd
pause