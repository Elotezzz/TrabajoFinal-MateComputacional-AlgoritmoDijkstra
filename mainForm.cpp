#include "mainForm.h"

using namespace System;
using namespace System::Windows::Forms;

int main(array<String^>^ args)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);

    // Mostrar pantalla de presentación primero
    TrabajoFinalMateComputacionalAlgoritmoDijkstra::mainForm inicio;
    Application::Run(% inicio);
    return 0;
}