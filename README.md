https://www.wxwidgets.org

MAC->
g++ mro_wx_enhanced.cpp -std=c++17 `wx-config --cxxflags --libs` -o mro_wx_enhanced

WİNDOWS->
g++ mro_wx_enhanced.cpp -std=c++17 -I path/to/wxWidgets/include -L path/to/wxWidgets/lib -lwx_baseu-3.2 -lwx_mswu_core-3.2 -o mro_wx_enhanced

Bu kodu terminalde çalıştırdığında ana proje derlenir.

./mro_wx_enhanced
Projeyi çalıştırmak için bunu terminalde çalıştırman gerekiyor.
