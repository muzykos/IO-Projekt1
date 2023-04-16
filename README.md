# SO-Projekt1
1. Demon monitorujący katalog

Celem usługi jest monitorowanie zmian plików w podanym katalogu. Program startowy otrzymuje co najmniej dwa argumenty: ścieżkę źródłową i ścieżkę docelowa . Jeżeli któraś ze ścieżek nie jest katalogiem program powraca natychmiast z komunikatem błędu. W przeciwnym wypadku staje się demonem. Demon wykonuje następujące czynności: 

    przy pierwszym uruchomieniu skanuje katalog w poszukiwaniu plików i tworzy strukturę danych zawierającą ich aktualny stan (więcej poniżej);
    proces demon usypia domyślnie na pięć minut (czas spania można zmieniać przy pomocy dodatkowego opcjonalnego argumentu);
    po obudzeniu skanuje ponownie katalog i porównuje aktualny stan plików ze stanem zapisanym na starcie, wykonuje niezbędne operacje i usypia ponownie.

Możliwe jest natychmiastowe obudzenie się demona poprzez wysłanie mu sygnału SIGUSR1. Komunikaty demona, jak informacja o każdej akcji typu uśpienie/obudzenie się (naturalne lub w wyniku sygnału) czy wykonanie operacji na plikach, są przesyłane do logu systemowego (syslog). Operacje porównania stanu działają wg następujących zasad:

    napotkanie nowego plik w monitorowanym katalogu powinno spowodować zalogowanie informacji o nim do logu systemowego, podobnie w przypadku zniknięcia pliku istniejącego na starcie;
    w przypadku zmiany daty modyfikacji lub rozmiaru istniejącego już pliku, jego nowa wersja powinna być skopiowana do katalogu docelowego; 
    demon powinien zaktualizować informacje o zmodyfikowanym pliku, aby przy kolejnym obudzeniu nie trzeba było wykonać kopii (chyba że plik w katalogu źródłowym zostanie ponownie zmieniony);
    pozycje, które nie są zwykłymi plikami są ignorowane (np. katalogi i dowiązania symboliczne);
    operacje kopiowania mają być wykonane za pomocą niskopoziomowych operacji read/write.

Wersja podstawowa: 12p. Dodatkowo:

    Zamiast wielkości i daty modyfikacji demon sprawdza sumy kontrolne plików (np. algorytmem z rodziny SHA). Uwaga: można skorzystać tutaj z bibliotek zewnętrznych (np. biblioteki openssl), natomiast same operacje odczytu plików powinny opierać się na API niskopoziomowym (4p).

    Opcja -R pozwalająca na rekurencyjną synchronizację katalogów (teraz pozycje będące katalogami nie są ignorowane). Przy kopiowaniu zmienionych plików struktura katalogów powinna być zachowana w katalogu docelowym (8p)

    W zależności od rozmiaru plików dla małych plików wykonywane jest kopiowanie przy pomocy read/write a w przypadku dużych używany jest bardziej efektywny mechanizm, np.: przy pomocy mmap/write (plik źródłowy zostaje zamapowany w całości w pamięci) lub za pomocą dedykowanego wywołania (np. sendfile czy copy_file_range). Próg dzielący pliki małe od dużych może być przekazywany jako opcjonalny argument. Duże pliki powinny być generowane za pomocą tego samego programu, oddzielnego programu/skryptu lub bezpośrednio z użyciem narzędzia make. Wykonaj analizę wpływu danej metody kopiowania na szybkość kopiowania plików i przedstaw wyniki w dokumentacji (począwszy od plików >10MB). (10p)
