@echo off
rem El IDE del REPO (la version en curso). Arranca desde la raiz del repo para que el
rem BpVM.cfg que vea sea el del repo, no el de la carpeta desde la que se invoque.
cd /d C:\lenguajes\pm
java -jar C:\lenguajes\pm\BpIde\target\BpIde-6.0.jar
