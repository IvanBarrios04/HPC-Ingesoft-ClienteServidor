from abc import ABC, abstractmethod

# ==========================================
# SINGLETON - Configuración Centralizada
# ==========================================
class MonitoringConfig:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(MonitoringConfig, cls).__new__(cls)
            cls._instance.cpu_threshold = 80.0
            cls._instance.memory_threshold = 85.0
        return cls._instance


# ==========================================
# OBSERVER - Notificación de Alertas
# ==========================================
class Observer(ABC):
    @abstractmethod
    def update(self, message: str):
        pass

class EmailAlert(Observer):
    def update(self, message: str):
        print(f"[ALERT - EMAIL] {message}")

class SlackAlert(Observer):
    def update(self, message: str):
        print(f"[ALERT - SLACK] {message}")


# ==========================================
# ADAPTER - Integración de APIs Externas
# ==========================================
class LegacyMetricApi:
    """API externa legada con un formato de datos diferente"""
    def get_legacy_cpu_data(self):
        return {"cpu_usage_percent": 85.5}

    def get_legacy_memory_data(self):
        return {"memory_usage_percent": 70.0}

class MetricTarget(ABC):
    @abstractmethod
    def get_cpu_usage(self) -> float:
        pass

    @abstractmethod
    def get_memory_usage(self) -> float:
        pass

class LegacyApiAdapter(MetricTarget):
    """Adapta LegacyMetricApi (formato heterogéneo) a la interfaz MetricTarget."""
    def __init__(self, legacy_api: LegacyMetricApi):
        self.legacy_api = legacy_api

    def get_cpu_usage(self) -> float:
        data = self.legacy_api.get_legacy_cpu_data()
        return data["cpu_usage_percent"]

    def get_memory_usage(self) -> float:
        data = self.legacy_api.get_legacy_memory_data()
        return data["memory_usage_percent"]

class NativeMetricService(MetricTarget):
    """Microservicio que ya expone métricas en el formato nativo, sin necesitar adaptación."""
    def __init__(self, cpu_usage: float, memory_usage: float):
        self.cpu_usage = cpu_usage
        self.memory_usage = memory_usage

    def get_cpu_usage(self) -> float:
        return self.cpu_usage

    def get_memory_usage(self) -> float:
        return self.memory_usage


# ==========================================
# FACADE - Interfaz Simplificada de Monitoreo
# ==========================================
class MonitoringFacade:
    def __init__(self):
        self.config = MonitoringConfig()
        self.observers = []
        self.services: dict[str, MetricTarget] = {}

    def attach(self, observer: Observer):
        if observer not in self.observers:
            self.observers.append(observer)

    def notify(self, message: str):
        for obs in self.observers:
            obs.update(message)

    def register_service(self, name: str, metric_source: MetricTarget):
        """Registra un microservicio (o su Adapter) para ser incluido en el monitoreo."""
        self.services[name] = metric_source

    def check_metrics(self, service_name: str, metric_source: MetricTarget):
        cpu_usage = metric_source.get_cpu_usage()
        memory_usage = metric_source.get_memory_usage()
        print(f"[{service_name}] CPU: {cpu_usage}% (Umbral: {self.config.cpu_threshold}%) | "
              f"Memoria: {memory_usage}% (Umbral: {self.config.memory_threshold}%)")

        if cpu_usage > self.config.cpu_threshold:
            self.notify(f"¡Alerta! [{service_name}] Uso excesivo de CPU: {cpu_usage}%")
        if memory_usage > self.config.memory_threshold:
            self.notify(f"¡Alerta! [{service_name}] Uso excesivo de Memoria: {memory_usage}%")

    def run_monitoring_cycle(self):
        """Punto único de entrada: recorre todos los servicios registrados y evalúa sus métricas."""
        print(f"\n=== Ciclo de monitoreo ({len(self.services)} servicio(s) registrado(s)) ===")
        for name, metric_source in self.services.items():
            self.check_metrics(name, metric_source)


# ==========================================
# EJEMPLO DE USO
# ==========================================
if __name__ == "__main__":
    monitoreo = MonitoringFacade()

    # Agregar alertas (Observer)
    monitoreo.attach(EmailAlert())
    monitoreo.attach(SlackAlert())

    # Servicio legado, requiere Adapter para exponer la interfaz MetricTarget
    legacy_api = LegacyMetricApi()
    monitoreo.register_service("auth-service (legacy)", LegacyApiAdapter(legacy_api))

    # Microservicio nativo, ya cumple la interfaz MetricTarget directamente
    monitoreo.register_service("orders-service", NativeMetricService(cpu_usage=45.0, memory_usage=90.0))

    # Monitorear todos los servicios registrados en un único ciclo
    monitoreo.run_monitoring_cycle()


'''
PREGUNTAS

1. Garantiza que exista una única instancia global de la configuración (MonitoringConfig). Esto evita
que distintas partes del sistema consulten o modifiquen umbrales de alerta desalineados (por ejemplo,
que un servicio use 80% de límite de CPU y otro 90%).

2. Mediante el patrón Adapter. En lugar de modificar el núcleo del sistema para adaptarlo a los JSONs o
respuestas heterogéneas de APIs externas (como servicios legados), el Adapter envuelve la API externa y
la transforma a la interfaz que el sistema espera.

3. Oculta la complejidad técnica de coordinar la lectura de métricas, la verificación contra Singleton y
la notificación por Observer. Al cliente solo se le expone un punto de entrada sencillo
(register_service / run_monitoring_cycle), reduciendo la cantidad de dependencias que debe conocer.

4. Gracias a las interfaces desacopladas: cada microservicio solo necesita implementar o adaptarse a
MetricTarget para ser registrado en la Facade. No hay que escribir lógica condicional adicional
por cada nuevo microservicio que se añada al ecosistema; basta con llamar a register_service().

5. En el módulo principal de monitoreo. Si no se usaran patrones, ese módulo tendría sentencias if/else
para convertir manualmente la respuesta de cada API externa, llamadas directas hardcodeadas a Slack/Email,
e instanciación local de parámetros de configuración.
'''
