#pragma once
#include "Algoritmo_Djikstra.h"
#include <climits>
#include <vector>
#include <queue>
#include <limits>
#include <sstream>

namespace ProyectoMateComputacional {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Text;
	using namespace Microsoft::VisualBasic;

	/// <summary>
	/// Resumen de Prueba
	/// </summary>
	public ref class Prueba : public System::Windows::Forms::Form
	{
	private:
		Grafo* grafo;
		int origen = -1;
		int destino = -1;

		// Animacion de creacion de nodos
		int visibleNodes = 0;
		System::Windows::Forms::Timer^ nodeTimer;

		// Para resaltar aristas del resultado de Dijkstra
		System::Collections::Generic::List<System::Tuple<int, int>^>^ highlightedEdges;

		// Para guardar múltiples caminos con colores asociados
		System::Collections::Generic::List<System::Drawing::Color>^ pathColors;
		System::Collections::Generic::List<System::Collections::Generic::List<System::Tuple<int, int>^>^>^ allHighlightedPaths;

		// Para animacion de aristas y control de visibilidad (managed)
		array<array<bool>^>^ visibleEdges;
		System::Collections::Generic::List<System::Tuple<int, int>^>^ pendingEdges;
		int edgeRevealIndex = 0;
		bool animatingEdges = false;

		// Controles para Dijkstra (panel derecho)
		System::Windows::Forms::ComboBox^ cmbDijOrigen;
		System::Windows::Forms::ComboBox^ cmbDijDestino;
		System::Windows::Forms::Button^ btnRunDijkstra;
		System::Windows::Forms::TextBox^ txtDijkstraInfo;
		System::Windows::Forms::Label^ lblDijOrigen;
		System::Windows::Forms::Label^ lblDijDestino;
	public:
		Prueba(void)
		{
			InitializeComponent();
			// Inicializamos el puntero de nuestro grafo
			srand((unsigned)time(nullptr)); //Para aleatorizar GG
			grafo = new Grafo();


			this->DoubleBuffered = true;
			this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
				ControlStyles::UserPaint |
				ControlStyles::AllPaintingInWmPaint, true);
			this->UpdateStyles();
		}

		// Permite seleccionar modo desde otra ventana
		void SetMode(bool manual) {
			this->rbManual->Checked = manual;
			this->rbAleatorio->Checked = !manual;
			// ocultar selección de modo porque ya fue elegida
			this->rbManual->Visible = false;
			this->rbAleatorio->Visible = false;
			this->lblMode->Visible = true;
			this->lblMode->Text = manual ? "Modo: Manual" : "Modo: Aleatorio";
			// mostrar indicación para generar desde matriz si es manual
			this->lblMatrixHint->Visible = manual;
		}

		// Ejecutar Dijkstra y mostrar información del camino mínimo (solo el camino, sin iteraciones)
	private: System::Void btnRunDijkstra_Click(System::Object^ sender, System::EventArgs^ e) {
		int n = grafo->ObtenerNumNodos();
		if (n == 0) { MessageBox::Show("Grafo vacío."); return; }
		int s = -1, t = -1;
		if (this->cmbDijOrigen->SelectedIndex >= 0) s = this->cmbDijOrigen->SelectedIndex;
		if (this->cmbDijDestino->SelectedIndex >= 0) t = this->cmbDijDestino->SelectedIndex;
		if (s == -1 || t == -1) { MessageBox::Show("Selecciona origen y destino para Dijkstra."); return; }
		if (s == t) { MessageBox::Show("Origen y destino deben ser distintos."); return; }

		// Usar el método que encuentra TODOS los caminos mínimos
		CaminosMinimos resultado = grafo->ObtenerCaminosMinimos(s, t);

		std::stringstream log;

		if (resultado.caminos.empty()) {
			log << "No existe camino desde el vértice " << s << " hasta el vértice " << t << ".";
		}
		else {
			log << "Camino(s) mínimo(s) desde vértice " << s << " hasta vértice " << t << ":\r\n";
			log << "Distancia mínima: " << resultado.distancia << "\r\n\r\n";

			if (resultado.caminos.size() == 1) {
				log << "Camino (único): ";
				for (size_t i = 0; i < resultado.caminos[0].size(); ++i) {
					log << resultado.caminos[0][i];
					if (i + 1 < resultado.caminos[0].size()) log << " -> ";
				}
				log << "\r\n";
			}
			else {
				log << "Se encontraron " << resultado.caminos.size() << " caminos mínimos de igual distancia:\r\n\r\n";
				for (size_t c = 0; c < resultado.caminos.size(); ++c) {
					log << "Camino " << (c + 1) << ": ";
					for (size_t i = 0; i < resultado.caminos[c].size(); ++i) {
						log << resultado.caminos[c][i];
						if (i + 1 < resultado.caminos[c].size()) log << " -> ";
					}
					log << "\r\n";
				}
			}
		}

		this->txtDijkstraInfo->Text = gcnew String(log.str().c_str());

		// Resaltar todos los caminos con colores diferentes
		this->pathColors = gcnew System::Collections::Generic::List<System::Drawing::Color>();
		this->allHighlightedPaths = gcnew System::Collections::Generic::List<System::Collections::Generic::List<System::Tuple<int, int>^>^>();

		// Colores para los caminos (primero rojo, luego azul, luego verde, etc.)
		array<System::Drawing::Color>^ colors = gcnew array<System::Drawing::Color> {
			System::Drawing::Color::Red,
				System::Drawing::Color::Blue,
				System::Drawing::Color::Green,
				System::Drawing::Color::Orange,
				System::Drawing::Color::Purple
		};

		for (size_t c = 0; c < resultado.caminos.size(); ++c) {
			System::Collections::Generic::List<System::Tuple<int, int>^>^ camino_edges = gcnew System::Collections::Generic::List<System::Tuple<int, int>^>();
			for (size_t i = 1; i < resultado.caminos[c].size(); ++i) {
				camino_edges->Add(System::Tuple::Create(resultado.caminos[c][i - 1], resultado.caminos[c][i]));
			}
			this->allHighlightedPaths->Add(camino_edges);
			this->pathColors->Add(colors[c % colors->Length]);
		}

		this->Panel_Grafo->Refresh();
	}

		   // Muestra un cuadro de diálogo simple para pedir una cadena al usuario.
	private: String^ PromptInput(String^ message, String^ title, String^ defaultVal) {
		Form^ f = gcnew Form();
		f->Text = title;
		f->StartPosition = FormStartPosition::CenterParent;
		f->Size = System::Drawing::Size(320, 150);
		f->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		f->MaximizeBox = false;
		f->MinimizeBox = false;

		Label^ lbl = gcnew Label();
		lbl->Text = message;
		lbl->AutoSize = true;
		lbl->Location = System::Drawing::Point(10, 10);
		f->Controls->Add(lbl);

		TextBox^ tb = gcnew TextBox();
		tb->Text = defaultVal;
		tb->Location = System::Drawing::Point(10, 40);
		tb->Width = 280;
		f->Controls->Add(tb);

		Button^ ok = gcnew Button();
		ok->Text = "OK";
		ok->DialogResult = System::Windows::Forms::DialogResult::OK;
		ok->Location = System::Drawing::Point(80, 75);
		f->Controls->Add(ok);

		Button^ cancel = gcnew Button();
		cancel->Text = "Cancelar";
		cancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
		cancel->Location = System::Drawing::Point(170, 75);
		f->Controls->Add(cancel);

		f->AcceptButton = ok;
		f->CancelButton = cancel;

		if (f->ShowDialog() == System::Windows::Forms::DialogResult::OK) return tb->Text; else return nullptr;
	}

	protected:
		/// <summary>
		/// Limpiar los recursos que se estén usando.
		/// </summary>
		~Prueba()
		{

			if (components)
			{
				delete components;
			}
		}

		// Timer tick para animar la aparicion de nodos
	private: System::Void nodeTimer_Tick(System::Object^ sender, System::EventArgs^ e) {
		int n = grafo->ObtenerNumNodos();
		if (visibleNodes < n) {
			visibleNodes++;
			this->Panel_Grafo->Refresh();
		}
		else {
			// empezar a revelar aristas si hay
			if (!animatingEdges) {
				animatingEdges = true;
				this->edgeRevealIndex = 0;
				// si pendingEdges vacío, construirlo desde la matriz
				if (this->pendingEdges == nullptr || this->pendingEdges->Count == 0) {
					const std::vector<std::vector<int>>& mat = grafo->ObtenerMatriz();
					if (this->pendingEdges == nullptr) this->pendingEdges = gcnew System::Collections::Generic::List<System::Tuple<int, int>^>();
					for (int i = 0; i < n; ++i) for (int j = 0; j < n; ++j) if (mat[i][j] != 0) this->pendingEdges->Add(System::Tuple::Create(i, j));
				}
				// si no hay aristas, terminar anim
				if (this->pendingEdges == nullptr || this->pendingEdges->Count == 0) {
					this->nodeTimer->Stop();
					// llenar solo los combos de Dijkstra (panel derecho)
					this->cmbDijOrigen->Items->Clear();
					this->cmbDijDestino->Items->Clear();
					for (int i = 0; i < n; ++i) { this->cmbDijOrigen->Items->Add(i.ToString()); this->cmbDijDestino->Items->Add(i.ToString()); }
					return;
				}
			}
			// revelar siguiente arista
			if (animatingEdges && this->pendingEdges != nullptr && edgeRevealIndex < this->pendingEdges->Count) {
				System::Tuple<int, int>^ pr = this->pendingEdges[edgeRevealIndex++];
				int u = pr->Item1; int v = pr->Item2;
				if (this->visibleEdges != nullptr && this->visibleEdges->Length == n) this->visibleEdges[u][v] = true;
				this->Panel_Grafo->Refresh();
				return;
			}
			// terminado
			this->nodeTimer->Stop();
			// llenar solo los combos del panel derecho (Dijkstra)
			this->cmbDijOrigen->Items->Clear();
			this->cmbDijDestino->Items->Clear();
			for (int i = 0; i < n; ++i) { this->cmbDijOrigen->Items->Add(i.ToString()); this->cmbDijDestino->Items->Add(i.ToString()); }
		}
	}

	private: System::Windows::Forms::NumericUpDown^ txtNodos;
	private: System::Windows::Forms::Button^ Btn_Generar_Grafo;
	private: System::Windows::Forms::RadioButton^ rbAleatorio;
	private: System::Windows::Forms::RadioButton^ rbManual;
	private: System::Windows::Forms::TextBox^ txtManualInput;
	private: System::Windows::Forms::Panel^ Panel_Grafo;
	private: array<System::Drawing::Point>^ nodosPos;
	private: int radioNodoMember = 18;
	private: int seleccionadoOrigenClick = -1;
	private: System::Windows::Forms::Label^ lblAviso;
		   // Left-side origin/destination controls removed (Dijkstra controls are on the right)
	private: System::Windows::Forms::Label^ lblMode;
	private: System::Windows::Forms::Label^ lblMatrixHint;
	protected:

	private:
		/// <summary>
		/// Variable del diseñador necesaria.
		/// </summary>
		System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Método necesario para admitir el Diseñador. No se puede modificar
		/// el contenido de este método con el editor de código.
		/// </summary>
		void InitializeComponent(void)
		{
			this->txtNodos = (gcnew System::Windows::Forms::NumericUpDown());
			this->Btn_Generar_Grafo = (gcnew System::Windows::Forms::Button());
			this->rbAleatorio = (gcnew System::Windows::Forms::RadioButton());
			this->rbManual = (gcnew System::Windows::Forms::RadioButton());
			this->txtManualInput = (gcnew System::Windows::Forms::TextBox());
			this->Panel_Grafo = (gcnew System::Windows::Forms::Panel());
			this->lblAviso = (gcnew System::Windows::Forms::Label());

			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->txtNodos))->BeginInit();
			this->SuspendLayout();
			// 
			// txtNodos
			// 
			this->txtNodos->Location = System::Drawing::Point(27, 106);
			this->txtNodos->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 16, 0, 0, 0 });
			this->txtNodos->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 7, 0, 0, 0 });
			this->txtNodos->Name = L"txtNodos";
			this->txtNodos->Size = System::Drawing::Size(120, 22);
			this->txtNodos->TabIndex = 1;
			this->txtNodos->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 7, 0, 0, 0 });
			this->txtNodos->ValueChanged += gcnew System::EventHandler(this, &Prueba::txtNodos_ValueChanged);
			// 
			// rbAleatorio
			// 
			this->rbAleatorio->AutoSize = true;
			this->rbAleatorio->Location = System::Drawing::Point(27, 40);
			this->rbAleatorio->Name = L"rbAleatorio";
			this->rbAleatorio->Size = System::Drawing::Size(80, 21);
			this->rbAleatorio->TabIndex = 0;
			this->rbAleatorio->TabStop = true;
			this->rbAleatorio->Text = L"Aleatorio";
			this->rbAleatorio->UseVisualStyleBackColor = true;
			this->rbAleatorio->Checked = true;
			// 
			// rbManual
			// 
			this->rbManual->AutoSize = true;
			this->rbManual->Location = System::Drawing::Point(27, 60);
			this->rbManual->Name = L"rbManual";
			this->rbManual->Size = System::Drawing::Size(70, 21);
			this->rbManual->TabIndex = 0;
			this->rbManual->TabStop = true;
			this->rbManual->Text = L"Manual";
			this->rbManual->UseVisualStyleBackColor = true;
			this->rbManual->CheckedChanged += gcnew System::EventHandler(this, &Prueba::rbManual_CheckedChanged);
			// 
			// txtManualInput
			// 
			this->txtManualInput->Location = System::Drawing::Point(12, 360);
			this->txtManualInput->Multiline = true;
			this->txtManualInput->Name = L"txtManualInput";
			this->txtManualInput->ScrollBars = System::Windows::Forms::ScrollBars::Both;
			this->txtManualInput->Size = System::Drawing::Size(140, 150);
			this->txtManualInput->TabIndex = 10;
			this->txtManualInput->Visible = false;
			this->txtManualInput->AcceptsTab = true;
			this->txtManualInput->WordWrap = false;
			// lblMatrixHint (mensaje para generar desde matriz)
			this->lblMatrixHint = (gcnew System::Windows::Forms::Label());
			this->lblMatrixHint->Location = System::Drawing::Point(12, 335);
			this->lblMatrixHint->AutoSize = true;
			this->lblMatrixHint->Text = L"Generar el grafo a traves de matriz";
			this->lblMatrixHint->Visible = false;
			this->Controls->Add(this->lblMatrixHint);

			// lblMode (muestra el modo seleccionado)
			this->lblMode = (gcnew System::Windows::Forms::Label());
			this->lblMode->Location = System::Drawing::Point(27, 12);
			this->lblMode->AutoSize = true;
			this->lblMode->Visible = false;
			this->Controls->Add(this->lblMode);
			// 
			// Btn_Generar_Grafo
			// 
			this->Btn_Generar_Grafo->Location = System::Drawing::Point(45, 166);
			this->Btn_Generar_Grafo->Name = L"Btn_Generar_Grafo";
			this->Btn_Generar_Grafo->Size = System::Drawing::Size(75, 23);
			this->Btn_Generar_Grafo->TabIndex = 2;
			this->Btn_Generar_Grafo->Text = L"Presioname";
			this->Btn_Generar_Grafo->UseVisualStyleBackColor = true;
			this->Btn_Generar_Grafo->Click += gcnew System::EventHandler(this, &Prueba::Btn_Generar_Grafo_Click);
			// 
	  // nodeTimer para animacion de nodos
			this->nodeTimer = (gcnew System::Windows::Forms::Timer());
			this->nodeTimer->Interval = 250; // ms
			this->nodeTimer->Tick += gcnew System::EventHandler(this, &Prueba::nodeTimer_Tick);

			// Panel_Grafo
			// 
			this->Panel_Grafo->Location = System::Drawing::Point(153, 12);
			this->Panel_Grafo->Name = L"Panel_Grafo";
			this->Panel_Grafo->Size = System::Drawing::Size(913, 595);
			this->Panel_Grafo->TabIndex = 3;
			this->Panel_Grafo->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &Prueba::Panel_Grafo_Paint);
			this->Panel_Grafo->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &Prueba::Panel_Grafo_MouseClick);
			// 
			// lblAviso
			// 
			this->lblAviso->AutoSize = true;
			this->lblAviso->Location = System::Drawing::Point(24, 85);
			this->lblAviso->Name = L"lblAviso";
			this->lblAviso->Size = System::Drawing::Size(180, 17);
			this->lblAviso->TabIndex = 0;
			this->lblAviso->Text = L"Mínimo permitido: 7, Máximo permitido: 16";
			// 

			// Prueba
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1122, 653);
			this->Controls->Add(this->lblAviso);
			this->Controls->Add(this->Panel_Grafo);
			this->Controls->Add(this->Btn_Generar_Grafo);
			this->Controls->Add(this->txtNodos);
			this->Controls->Add(this->rbAleatorio);
			this->Controls->Add(this->rbManual);
			this->Controls->Add(this->txtManualInput);
			// Crear controles del panel derecho para Dijkstra
			this->cmbDijOrigen = (gcnew System::Windows::Forms::ComboBox());
			this->cmbDijDestino = (gcnew System::Windows::Forms::ComboBox());
			this->btnRunDijkstra = (gcnew System::Windows::Forms::Button());
			this->txtDijkstraInfo = (gcnew System::Windows::Forms::TextBox());

			this->cmbDijOrigen->Location = System::Drawing::Point(1100, 40);
			this->cmbDijOrigen->Size = System::Drawing::Size(150, 24);
			this->cmbDijDestino->Location = System::Drawing::Point(1100, 80);
			this->cmbDijDestino->Size = System::Drawing::Size(150, 24);
			this->btnRunDijkstra->Location = System::Drawing::Point(1100, 120);
			this->btnRunDijkstra->Size = System::Drawing::Size(150, 30);
			this->btnRunDijkstra->Text = L"Ejecutar Dijkstra";
			this->btnRunDijkstra->Click += gcnew System::EventHandler(this, &Prueba::btnRunDijkstra_Click);
			this->txtDijkstraInfo->Location = System::Drawing::Point(1100, 160);
			this->txtDijkstraInfo->Size = System::Drawing::Size(300, 300);
			this->txtDijkstraInfo->Multiline = true;
			this->txtDijkstraInfo->ScrollBars = System::Windows::Forms::ScrollBars::Both;

			this->Controls->Add(this->cmbDijOrigen);
			this->Controls->Add(this->cmbDijDestino);
			this->Controls->Add(this->btnRunDijkstra);
			this->Controls->Add(this->txtDijkstraInfo);

			// labels for Dijkstra combos
			this->lblDijOrigen = (gcnew System::Windows::Forms::Label());
			this->lblDijDestino = (gcnew System::Windows::Forms::Label());
			this->lblDijOrigen->Location = System::Drawing::Point(1100, 20);
			this->lblDijOrigen->AutoSize = true;
			this->lblDijOrigen->Text = L"Vértice origen (Dijkstra):";
			this->lblDijDestino->Location = System::Drawing::Point(1100, 60);
			this->lblDijDestino->AutoSize = true;
			this->lblDijDestino->Text = L"Vértice destino (Dijkstra):";
			this->Controls->Add(this->lblDijOrigen);
			this->Controls->Add(this->lblDijDestino);
			this->Name = L"Prueba";
			this->Text = L"Prueba";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->txtNodos))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void txtNodos_ValueChanged(System::Object^ sender, System::EventArgs^ e) {

	}

	private: System::Void Btn_Generar_Grafo_Click(System::Object^ sender, System::EventArgs^ e) {
		int n = safe_cast<int>(this->txtNodos->Value);
		if (this->rbManual->Checked) {
			// Leer matriz desde el textbox (cada fila en una línea, valores separados por espacios)
			String^ texto = this->txtManualInput->Text->Trim();
			if (String::IsNullOrWhiteSpace(texto)) {
				// Si el textbox está vacío, creamos un grafo sin aristas y animamos la aparición de nodos
				std::vector<std::vector<int>> mat(n, std::vector<int>(n, 0));
				grafo->CargarDesdeMatriz(mat);
				this->visibleNodes = 0;
				this->visibleEdges = gcnew array<array<bool>^>(n);
				for (int ii = 0; ii < n; ++ii) { this->visibleEdges[ii] = gcnew array<bool>(n); for (int jj = 0; jj < n; ++jj) this->visibleEdges[ii][jj] = false; }
				this->pendingEdges = gcnew System::Collections::Generic::List<System::Tuple<int, int>^>();
				this->edgeRevealIndex = 0;
				this->animatingEdges = false;
				this->nodeTimer->Start();
				MessageBox::Show("Grafo vacío creado. Usa los clicks sobre los nodos para añadir aristas sin crear ciclos.");
				return;
			}
			else {
				array<String^>^ filas = texto->Split(gcnew array<wchar_t>{ '\n' }, StringSplitOptions::RemoveEmptyEntries);
				if ((int)filas->Length != n) {
					MessageBox::Show("La cantidad de filas no coincide con el número de nodos.");
					return;
				}
				std::vector<std::vector<int>> mat(n, std::vector<int>(n, 0));
				for (int i = 0; i < n; i++) {
					String^ linea = filas[i]->Trim();
					array<String^>^ tokens = linea->Split(gcnew array<wchar_t>{ ' ', '\t', ',' }, StringSplitOptions::RemoveEmptyEntries);
					if ((int)tokens->Length != n) {
						MessageBox::Show("Cada fila debe contener exactamente " + n.ToString() + " valores.");
						return;
					}
					for (int j = 0; j < n; j++) {
						try {
							int val = Int32::Parse(tokens[j]);
							mat[i][j] = val;
						}
						catch (...) {
							MessageBox::Show("Valor inválido en la matriz. Usa enteros.");
							return;
						}
					}
				}

				if (!grafo->CargarDesdeMatriz(mat)) {
					MessageBox::Show("La matriz contiene ciclos o no es válida. El grafo debe ser acíclico.");
					return;
				}
				this->visibleNodes = n;
				// marcar todas las aristas como visibles directamente (managed)
				this->visibleEdges = gcnew array<array<bool>^>(n);
				for (int ii = 0; ii < n; ++ii) { this->visibleEdges[ii] = gcnew array<bool>(n); for (int jj = 0; jj < n; ++jj) this->visibleEdges[ii][jj] = (mat[ii][jj] != 0); }
			}

		}
		else {
			grafo->GenerarAleatorio(n);
			// Mostrar la matriz generada en el cuadro de texto para que el usuario la vea
			const std::vector<std::vector<int>>& mat = grafo->ObtenerMatriz();
			System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
			for (int i = 0; i < n; ++i) {
				for (int j = 0; j < n; ++j) {
					sb->Append(System::Convert::ToString(mat[i][j]));
					if (j < n - 1) sb->Append(" ");
				}
				if (i < n - 1) sb->AppendLine();
			}
			this->txtManualInput->Text = sb->ToString();
			this->txtManualInput->Visible = true;
			// mostrar nodos inmediatamente, preparar animacion solo para aristas
			this->visibleNodes = n; // no parpadeo de nodos
			this->visibleEdges = gcnew array<array<bool>^>(n);
			for (int ii = 0; ii < n; ++ii) { this->visibleEdges[ii] = gcnew array<bool>(n); for (int jj = 0; jj < n; ++jj) this->visibleEdges[ii][jj] = false; }
			this->pendingEdges = gcnew System::Collections::Generic::List<System::Tuple<int, int>^>();
			const std::vector<std::vector<int>>& mat2 = grafo->ObtenerMatriz();
			for (int i = 0; i < n; ++i) for (int j = 0; j < n; ++j) if (mat2[i][j] != 0) this->pendingEdges->Add(System::Tuple::Create(i, j));
			this->edgeRevealIndex = 0;
			this->animatingEdges = true; // empezar a revelar aristas
			this->nodeTimer->Start();
			// actualizar combos Dijkstra inmediatamente (nodos ya visibles)
			this->cmbDijOrigen->Items->Clear();
			this->cmbDijDestino->Items->Clear();
			for (int i = 0; i < n; ++i) { this->cmbDijOrigen->Items->Add(i.ToString()); this->cmbDijDestino->Items->Add(i.ToString()); }
		}

		Panel_Grafo->Refresh();

		// actualizar solo los combos del panel derecho (Dijkstra)
		this->cmbDijOrigen->Items->Clear();
		this->cmbDijDestino->Items->Clear();
		for (int i = 0; i < n; i++) {
			this->cmbDijOrigen->Items->Add(i.ToString());
			this->cmbDijDestino->Items->Add(i.ToString());
		}

		MessageBox::Show("¡Grafo generado con éxito!");
	}

	private: System::Void rbManual_CheckedChanged(System::Object^ sender, System::EventArgs^ e) {
		this->txtManualInput->Visible = this->rbManual->Checked;
		if (this->rbManual->Checked) {
			// Al pasar a modo manual, borrar grafo existente (si lo había)
			this->grafo->Vaciar();
			this->Panel_Grafo->Refresh();
			this->origen = -1;
			this->destino = -1;
		}
	}
	private: System::Void Panel_Grafo_Paint(System::Object^ sender, System::Windows::Forms::PaintEventArgs^ e) {
		int n = grafo->ObtenerNumNodos();
		if (n == 0) return;
		const std::vector<std::vector<int>>& matriz = grafo->ObtenerMatriz();

		Graphics^ g = e->Graphics;
		g->SmoothingMode = System::Drawing::Drawing2D::SmoothingMode::AntiAlias;

		int cx = Panel_Grafo->ClientSize.Width / 2;
		int cy = Panel_Grafo->ClientSize.Height / 2;
		int radio = Math::Min(cx, cy) - 40;
		int radioNodo = 18;

		array<Point>^ pos = gcnew array<Point>(n);
		for (int i = 0; i < n; i++) {
			double ang = 2 * Math::PI * i / n;
			pos[i] = Point(cx + (int)(radio * Math::Cos(ang)), cy + (int)(radio * Math::Sin(ang)));
		}

		// Guardar posiciones para manejo de click
		this->nodosPos = pos;
		this->radioNodoMember = radioNodo;

		Pen^ penArista = gcnew Pen(Color::DimGray, 2);
		penArista->CustomEndCap = gcnew System::Drawing::Drawing2D::AdjustableArrowCap(5, 6);

		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				if (matriz[i][j] != 0) {
					// si visibleEdges está inicializado, solo dibujar si está marcada visible
					if (this->visibleEdges != nullptr && this->visibleEdges->Length == n) {
						if (!this->visibleEdges[i][j]) continue;
					}
					// solo dibujar aristas entre nodos visibles
					if (visibleNodes > 0 && (i >= visibleNodes || j >= visibleNodes)) continue;
					double dx = pos[j].X - pos[i].X;
					double dy = pos[j].Y - pos[i].Y;
					double distancia = Math::Sqrt(dx * dx + dy * dy);

					if (distancia > 0) {
						int startX = pos[i].X + (int)((dx / distancia) * radioNodo);
						int startY = pos[i].Y + (int)((dy / distancia) * radioNodo);

						int endX = pos[j].X - (int)((dx / distancia) * radioNodo);
						int endY = pos[j].Y - (int)((dy / distancia) * radioNodo);

						Point pInicio(startX, startY);
						Point pFin(endX, endY);

						// buscar si esta arista está en alguno de los caminos resaltados
						int colorIndex = -1;
						if (this->allHighlightedPaths != nullptr) {
							for (int pathIdx = 0; pathIdx < this->allHighlightedPaths->Count; ++pathIdx) {
								System::Collections::Generic::List<System::Tuple<int, int>^>^ camino = this->allHighlightedPaths[pathIdx];
								for each (System::Tuple<int, int> ^ t in camino) {
									if (t->Item1 == i && t->Item2 == j) { colorIndex = pathIdx; break; }
								}
								if (colorIndex >= 0) break;
							}
						}

						if (colorIndex >= 0) {
							// resaltar con color del camino
							System::Drawing::Color pathColor = this->pathColors[colorIndex];
							Pen^ penR = gcnew Pen(pathColor, 4);
							penR->CustomEndCap = gcnew System::Drawing::Drawing2D::AdjustableArrowCap(6, 8);
							g->DrawLine(penR, pInicio, pFin);
						}
						else {
							g->DrawLine(penArista, pInicio, pFin);
						}

						Point medio((pInicio.X + pFin.X) / 2, (pInicio.Y + pFin.Y) / 2);
						g->DrawString(matriz[i][j].ToString(), this->Font, Brushes::Red, medio);
					}
				}
			}
		}
		for (int i = 0; i < n; i++) {
			Rectangle r(pos[i].X - radioNodo, pos[i].Y - radioNodo, radioNodo * 2, radioNodo * 2);
			if (this->seleccionadoOrigenClick == i) {
				g->FillEllipse(Brushes::LightGreen, r);
			}
			else {
				g->FillEllipse(Brushes::LightSkyBlue, r);
			}
			g->DrawEllipse(Pens::Black, r);
			String^ etq = i.ToString();
			SizeF tam = g->MeasureString(etq, this->Font);
			g->DrawString(etq, this->Font, Brushes::Black, pos[i].X - tam.Width / 2, pos[i].Y - tam.Height / 2);
		}
	}

	private: System::Void Panel_Grafo_MouseClick(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e) {
		if (!this->rbManual->Checked) return; // solo en modo manual
		int n = grafo->ObtenerNumNodos();
		if (n == 0) return;
		if (this->nodosPos == nullptr) return;
		// encontrar nodo clickeado
		int encontrado = -1;
		for (int i = 0; i < n; ++i) {
			Point p = this->nodosPos[i];
			double dx = e->X - p.X;
			double dy = e->Y - p.Y;
			double dist2 = dx * dx + dy * dy;
			double radio2 = (this->radioNodoMember * 1.5) * (this->radioNodoMember * 1.5);
			if (dist2 <= radio2) { encontrado = i; break; }
		}
		if (encontrado == -1) return;
		if (this->seleccionadoOrigenClick == -1) {
			// seleccionar origen
			this->seleccionadoOrigenClick = encontrado;
			this->Panel_Grafo->Refresh();
			return;
		}
		// ya había origen seleccionado
		int origenClick = this->seleccionadoOrigenClick;
		int destinoClick = encontrado;
		if (origenClick == destinoClick) {
			// deseleccionar
			this->seleccionadoOrigenClick = -1;
			this->Panel_Grafo->Refresh();
			return;
		}
		// pedir peso
		String^ input = this->PromptInput("Ingrese peso para la arista " + origenClick.ToString() + " -> " + destinoClick.ToString(), "Peso", "1");
		if (String::IsNullOrWhiteSpace(input)) {
			this->seleccionadoOrigenClick = -1;
			this->Panel_Grafo->Refresh();
			return;
		}
		int peso = 0;
		try {
			peso = Int32::Parse(input);
		}
		catch (...) {
			MessageBox::Show("Peso inválido. Usa un entero.");
			this->seleccionadoOrigenClick = -1;
			this->Panel_Grafo->Refresh();
			return;
		}
		// intentar agregar arista sin crear ciclo
		if (!grafo->AgregarAristaSiAcyclic(origenClick, destinoClick, peso)) {
			MessageBox::Show("No se puede agregar la arista: crearía un ciclo.");
			this->seleccionadoOrigenClick = -1;
			this->Panel_Grafo->Refresh();
			return;
		}
		// agregado correctamente
		 // marcar arista visible (si estructura inicializada)
		if (this->visibleEdges != nullptr && this->visibleEdges->Length == grafo->ObtenerNumNodos()) this->visibleEdges[origenClick][destinoClick] = true;
		this->seleccionadoOrigenClick = -1;
		this->Panel_Grafo->Refresh();
	}

	};
}