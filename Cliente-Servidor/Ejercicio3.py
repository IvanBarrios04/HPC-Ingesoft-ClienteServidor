from abc import ABC, abstractmethod

# ==========================================
# FACTORY METHOD - Creación de Usuarios
# ==========================================
class User(ABC):
    def __init__(self, name: str, role: str):
        self.name = name
        self.role = role

class Student(User):
    def __init__(self, name: str):
        super().__init__(name, "Student")

class Teacher(User):
    def __init__(self, name: str):
        super().__init__(name, "Teacher")

class UserFactory:
    @staticmethod
    def create_user(role: str, name: str) -> User:
        if role == "student":
            return Student(name)
        elif role == "teacher":
            return Teacher(name)
        else:
            raise ValueError("Rol no válido")


# ==========================================
# STRATEGY - Algoritmos de Evaluación
# ==========================================
class EvaluationStrategy(ABC):
    @abstractmethod
    def calculate_grade(self, grades: list) -> float:
        pass

class SimpleAverageStrategy(EvaluationStrategy):
    def calculate_grade(self, grades: list) -> float:
        return sum(grades) / len(grades) if grades else 0.0

class WeightedAverageStrategy(EvaluationStrategy):
    """Considera una tupla con (nota, peso)"""
    def calculate_grade(self, grades: list) -> float:
        total_score = sum(note * weight for note, weight in grades)
        total_weight = sum(weight for _, weight in grades)
        return total_score / total_weight if total_weight > 0 else 0.0


# ==========================================
# PROXY - Control de Acceso
# ==========================================
class CourseService(ABC):
    @abstractmethod
    def edit_course_content(self, user: User, content: str):
        pass

class RealCourseService(CourseService):
    def edit_course_content(self, user: User, content: str):
        print(f"Contenido actualizado por {user.name}: '{content}'")

class CourseProxy(CourseService):
    def __init__(self):
        self.real_service = RealCourseService()

    def edit_course_content(self, user: User, content: str):
        if user.role != "Teacher":
            print(f"[ACCESO DENEGADO] El usuario {user.name} ({user.role}) no tiene permisos para editar el curso.")
        else:
            self.real_service.edit_course_content(user, content)


# ==========================================
# OBSERVER - Notificaciones Académicas
# ==========================================
class LMSObserver(ABC):
    @abstractmethod
    def update(self, message: str):
        pass

class EmailNotifier(LMSObserver):
    def update(self, message: str):
        print(f"[EMAIL LMS] {message}")

class PushNotifier(LMSObserver):
    def update(self, message: str):
        print(f"[PUSH NOTIFICATION] {message}")


# ==========================================
# EJEMPLO DE USO
# ==========================================
if __name__ == "__main__":
    # Crear usuarios con Factory
    profesor = UserFactory.create_user("teacher", "Dr. Pérez")
    estudiante = UserFactory.create_user("student", "Juan")

    # Probar control de acceso con Proxy
    proxy = CourseProxy()
    proxy.edit_course_content(estudiante, "Nuevo tema de clase") # Fallará por permisos
    proxy.edit_course_content(profesor, "Syllabus Modificado")   # Permitido

    # Calcular nota con Strategy
    estrategia_promedio = SimpleAverageStrategy()
    notas = [4.0, 3.5, 5.0]
    final = estrategia_promedio.calculate_grade(notas)
    print(f"Nota final calculada: {final:.2f}")

    # Notificar eventos con Observer
    notificador = PushNotifier()
    notificador.update(f"Se ha publicado la nota final para {estudiante.name}: {final:.2f}")


'''
PREGUNTAS

1. Creando una nueva clase que implemente la interfaz EvaluationStrategy (por ejemplo, RubricStrategy).
No es necesario modificar las clases existentes de cursos ni el motor de calificaciones, respetando el 
principio Open/Closed.

2. Permite cambiar o añadir nuevos formatos (PDF, Excel, HTML) de manera independiente a la lógica del 
curso o del cálculo de notas. El sistema que procesa las calificaciones no necesita saber cómo se dibuja 
o renderiza un documento.

3. Porque aplica el principio de Responsabilidad Única (SRP). Evita llenar las clases de lógica de 
negocio con bloques if (user.role == "Teacher") dispersos por todo el código. El Proxy intercepta las 
llamadas y gestiona la seguridad antes de tocar el objeto real.

4. Delegando responsabilidades a través de patrones: la creación a las fábricas (UserFactory), los
cálculos a las estrategias (EvaluationStrategy) y la gestión de eventos al Observer. De este modo, la 
clase central solo coordina interfaces en lugar de implementar lógica detallada.

5. Al estar los componentes desacoplados, la corrección de errores o la adición de nuevas características
se limita a clases pequeñas y aisladas. Esto reduce drásticamente el riesgo de romper funcionalidades
existentes (efecto dominó) durante actualizaciones o pruebas.
'''