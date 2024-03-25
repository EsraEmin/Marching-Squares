--README--EMIN ESRA--332CC--

In fisierul "tema1_par.c" am creat o noua functie ("tema") si o structura de date ("hreadInfo") , cu campurile necesare modificarii programului , atfel incat sa utilizeze multi-threading.

Functia "tema" a preluat treaba functiilor "rescale_image", "sample_image" si "march" , deoarece aceasta este functia pe care o vor utiliza threadurile. In functia "tema" dupa ce am terminat de rescalat imaginea , am folosit o bariera "barrier1" pentru a fi sigura ca toate threadurile au trecut de prima parte a functiei. Acest lucru l-am facut si dupa crearea gridului binar , am folosit aceasi bariera pentru a nu creste timpul. Pentru a impartii in mod egal sarcinile intre threaduri , am calculat pe baza id-ului threadurilor un nou start/end , pentru partea cu crearea gridului binar si startt/endd , pentru partea de rescalare.

In functia "main" am utilizat functia "atoi" pentru a schimba argumentul[3] cu cate threaduri urmeaza sa fie folosite , care este un string in integer , am alocat memorie pentru imagini si am creat threadurile , dupa care le-am dat join. Dupa care am verificat daca imaginea este deja scalata sau nu , iar la final am eliberat resurele si am distrus bariera.





