from abc import ABC, abstractmethod


# SINGLETON (Configuración global)

class LMSConfig:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(LMSConfig, cls).__new__(cls)
            cls._instance.institution_name = "Universidad Tecnológica"
            cls._instance.academic_period = "2026-1"
        return cls._instance

    def get_institution_name(self) -> str:
        return self.institution_name

    def get_academic_period(self) -> str:
        return self.academic_period



# FACTORY METHOD  (Creación de Usuarios)

class User(ABC):
    def __init__(self, user_id: str, name: str, role: str):
        self.user_id = user_id
        self.name = name
        self.role = role

class Student(User):
    def __init__(self, user_id: str, name: str):
        super().__init__(user_id, name, "Student")

class Teacher(User):
    def __init__(self, user_id: str, name: str):
        super().__init__(user_id, name, "Teacher")

class Administrator(User):
    def __init__(self, user_id: str, name: str):
        super().__init__(user_id, name, "Admin")

class UserFactory:
    @staticmethod
    def create_user(role: str, user_id: str, name: str) -> User:
        role_clean = role.lower().strip()
        if role_clean == "student":
            return Student(user_id, name)
        elif role_clean == "teacher":
            return Teacher(user_id, name)
        elif role_clean in ["admin", "administrator"]:
            return Administrator(user_id, name)
        else:
            raise ValueError(f"Rol '{role}' no reconocido")


# FACTORY METHOD  (Plugins de Evaluación)

class Evaluation(ABC):
    def __init__(self, title: str, max_score: float):
        self.title = title
        self.max_score = max_score

    @abstractmethod
    def evaluate(self, submission_data: dict) -> float:
        pass

class SimpleQuiz(Evaluation):
    def evaluate(self, submission_data: dict) -> float:
        score = float(submission_data.get("score", 0.0))
        return min(score, self.max_score)

class RubricEvaluation(Evaluation):
    def evaluate(self, submission_data: dict) -> float:
        criteria = submission_data.get("criteria", [])
        return sum(criteria) / len(criteria) if criteria else 0.0

class EvaluationFactory:
    @staticmethod
    def create_evaluation(eval_type: str, title: str, max_score: float) -> Evaluation:
        eval_clean = eval_type.lower().strip()
        if eval_clean == "quiz":
            return SimpleQuiz(title, max_score)
        elif eval_clean == "rubric":
            return RubricEvaluation(title, max_score)
        else:
            raise ValueError(f"Tipo de evaluación '{eval_type}' no soportado")


# STRATEGY  (Formatos de Reportes Académicos)

class ReportStrategy(ABC):
    @abstractmethod
    def generate_report(self, academic_data: dict) -> str:
        pass

class PDFReportStrategy(ReportStrategy):
    def generate_report(self, academic_data: dict) -> str:
        return f"[PDF REPORT] Estudiante: {academic_data.get('student')} | Nota: {academic_data.get('grade')}"

class ExcelReportStrategy(ReportStrategy):
    def generate_report(self, academic_data: dict) -> str:
        return f"[EXCEL REPORT] {academic_data.get('student')},{academic_data.get('grade')}"

class HTMLReportStrategy(ReportStrategy):
    def generate_report(self, academic_data: dict) -> str:
        return f"<html><body><h1>{academic_data.get('student')}</h1><p>Nota: {academic_data.get('grade')}</p></body></html>"


# PROXY  (Control de Acceso por Rol)

class CourseService(ABC):
    @abstractmethod
    def edit_course_content(self, user: User, course_id: str, content: str) -> None:
        pass

class RealCourseService(CourseService):
    def edit_course_content(self, user: User, course_id: str, content: str) -> None:
        print(f"[CURSO {course_id}] Actualizado por {user.name}: '{content}'")

class CourseProxy(CourseService):
    def __init__(self, real_service: RealCourseService):
        self.real_service = real_service

    def edit_course_content(self, user: User, course_id: str, content: str) -> None:
        if user.role not in ["Teacher", "Admin"]:
            print(f"[ACCESO DENEGADO] {user.name} ({user.role}) no tiene permisos para editar contenidos.")
        else:
            self.real_service.edit_course_content(user, course_id, content)


# OBSERVER  (Notificaciones Académicas)

class LMSObserver(ABC):
    @abstractmethod
    def update(self, message: str) -> None:
        pass

class EmailNotifier(LMSObserver):
    def update(self, message: str) -> None:
        print(f"[EMAIL] {message}")

class PushNotifier(LMSObserver):
    def update(self, message: str) -> None:
        print(f"[PUSH] {message}")

class CourseSubject:
    def __init__(self, course_name: str):
        self.course_name = course_name
        self.observers: list[LMSObserver] = []

    def attach(self, observer: LMSObserver) -> None:
        if observer not in self.observers:
            self.observers.append(observer)

    def detach(self, observer: LMSObserver) -> None:
        self.observers.remove(observer)

    def notify(self, message: str) -> None:
        for obs in self.observers:
            obs.update(f"[{self.course_name}] {message}")


# REPOSITORY  (Persistencia de Datos)

class CourseRepository(ABC):
    @abstractmethod
    def save(self, course_data: dict) -> None:
        pass

class MemoryCourseRepository(CourseRepository):
    def __init__(self):
        self._storage: dict = {}

    def save(self, course_data: dict) -> None:
        course_id = course_data.get("id")
        self._storage[course_id] = course_data
        print(f"[REPOSITORY] Curso '{course_id}' guardado en memoria.")


# FACADE  (Punto de Entrada Centralizado)

class LMSFacade:
    def __init__(self):
        self.config = LMSConfig()
        self.course_proxy = CourseProxy(RealCourseService())
        self.course_repository = MemoryCourseRepository()

    # Operaciones delegadas
    def register_user(self, role: str, user_id: str, name: str) -> User:
        return UserFactory.create_user(role, user_id, name)

    def create_evaluation(self, eval_type: str, title: str, max_score: float) -> Evaluation:
        return EvaluationFactory.create_evaluation(eval_type, title, max_score)

    def update_course_content(self, user: User, course_id: str, content: str) -> None:
        self.course_proxy.edit_course_content(user, course_id, content)

    def generate_report(self, strategy: ReportStrategy, academic_data: dict) -> str:
        return strategy.generate_report(academic_data)


# Ejemplo de uso

if __name__ == "__main__":
    # Inicialización de la Fachada
    lms = LMSFacade()
    print(f"=== {lms.config.get_institution_name()} ({lms.config.get_academic_period()}) ===")

    # 1. Registro de usuarios mediante Factory
    profesor = lms.register_user("teacher", "T-100", "Dr. Óscar Sierra")
    estudiante = lms.register_user("student", "S-200", "Juan Pérez")

    # 2. Control de acceso mediante Proxy
    print("\n--- Control de Acceso (Proxy) ---")
    lms.update_course_content(estudiante, "IS924", "Intentando cambiar el Syllabus")  # Denegado
    lms.update_course_content(profesor, "IS924", "Nuevo tema: Patrones de Diseño")    # Permitido

    # 3. Creación y cálculo de Evaluaciones (Factory)
    print("\n--- Evaluación (Factory) ---")
    quiz = lms.create_evaluation("quiz", "Parcial 1", 5.0)
    nota_quiz = quiz.evaluate({"score": 4.5})

    rubrica = lms.create_evaluation("rubric", "Proyecto Final", 5.0)
    nota_rubrica = rubrica.evaluate({"criteria": [4.0, 5.0, 4.5]})
    print(f"Nota Quiz: {nota_quiz} | Nota Rúbrica: {nota_rubrica:.2f}")

    # 4. Eventos y Notificaciones (Observer)
    print("\n--- Notificaciones (Observer) ---")
    curso_is924 = CourseSubject("Arquitectura de Software")
    curso_is924.attach(EmailNotifier())
    curso_is924.attach(PushNotifier())
    curso_is924.notify(f"Notas publicadas para {estudiante.name}")

    # 5. Generación de Reportes (Strategy)
    print("\n--- Reportes (Strategy) ---")
    datos = {"student": estudiante.name, "grade": nota_quiz}
    print(lms.generate_report(PDFReportStrategy(), datos))
    print(lms.generate_report(ExcelReportStrategy(), datos))

    # 6. Guardar en Repositorio (Repository)
    print("\n--- Persistencia (Repository) ---")
    lms.course_repository.save({"id": "IS924", "name": "ACS", "teacher": profesor.name})
    
'''
PREGUNTAS

1. ¿Cómo se agregaría un nuevo tipo de evaluación?
Creando una nueva clase que herede de Evaluatio e implemente el método evaluate. Luego, se agrega 
la condición correspondiente dentro del EvaluationFactory. 

2. ¿Qué ventajas tiene desacoplar el formato de reporte?
Permite incorporar o modificar la forma de renderizar un informe (PDF, Excel, HTML, JSON) sin alterar los
modelos de datos ni la lógica del curso. La lógica de presentación queda totalmente separada del dominio de
negocio.

3. ¿Por qué Proxy es mejor que condicionales de rol?
Evita dispersar sentencias if user.role == ... por todos los métodos de la aplicación. El Proxy intercepta
las solicitudes en un solo punto, centraliza la política de seguridad y mantiene la lógica de negocio 
(RealCourseService) limpia e independiente del control de accesos.

4. ¿Cómo evitar que el sistema central crezca excesivamente?
Delegando las responsabilidades a subsistemas y patrones específicos: instanciación a los Factory, 
persistencia a Repository, notificaciones a Observer y algoritmos a Strategy. La clase central 
(LMSFacade) actúa como coordinadora sin acumular código de implementación.

5. ¿Cómo impacta el diseño en la mantenibilidad a largo plazo?
Reduce el acoplamiento y aísla los cambios. Al corregir errores o añadir nuevas funcionalidades, el impacto
se restringe únicamente a la clase encargada, evitando que afecte otras partes del
sistema y facilitando la realización de pruebas unitarias.
'''
