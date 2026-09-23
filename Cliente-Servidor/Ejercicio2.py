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

class MetricTarget(ABC):
    @abstractmethod
    def get_cpu_usage(self) -> float:
        pass

class LegacyApiAdapter(MetricTarget):
    def __init__(self, legacy_api: LegacyMetricApi):
        self.legacy_api = legacy_api

    def get_cpu_usage(self) -> float:
        data = self.legacy_api.get_legacy_cpu_data()
        return data["cpu_usage_percent"]


# ==========================================
# FACADE - Interfaz Simplificada de Monitoreo
# ==========================================
class MonitoringFacade:
    def __init__(self):
        self.config = MonitoringConfig()
        self.observers = []

    def attach(self, observer: Observer):
        self.observers.append(observer)

    def notify(self, message: str):
        for obs in self.observers:
            obs.update(message)

    def check_metrics(self, metric_source: MetricTarget):
        cpu_usage = metric_source.get_cpu_usage()
        print(f"Uso de CPU detectado: {cpu_usage}% (Umbral: {self.config.cpu_threshold}%)")
        if cpu_usage > self.config.cpu_threshold:
            self.notify(f"¡Alerta! Uso excesivo de CPU: {cpu_usage}%")


# ==========================================
# EJEMPLO DE USO
# ==========================================
if __name__ == "__main__":
    monitoreo = MonitoringFacade()

    # Agregar alertas (Observer)
    monitoreo.attach(EmailAlert())
    monitoreo.attach(SlackAlert())

    # Integrar métrica externa (Adapter)
    legacy_api = LegacyMetricApi()
    adapter = LegacyApiAdapter(legacy_api)

    # Monitorear
    monitoreo.check_metrics(adapter)


'''
PREGUNTAS

1. Garantiza que exista una única instancia global de la configuración (MonitoringConfig). Esto evita
que distintas partes del sistema consulten o modifiquen umbrales de alerta desalineados (por ejemplo, 
que un servicio use 80% de límite de CPU y otro 90%).

2. Mediante el patrón Adapter. En lugar de modificar el núcleo del sistema para adaptarlo a los JSONs o
respuestas heterogéneas de APIs externas (como servicios legados), el Adapter envuelve la API externa y
la transforma a la interfaz que el sistema espera.

3. Oculta la complejidad técnica de coordinar la lectura de métricas, la verificación contra Singleton y
la notificación por Observer. Al cliente solo se le expone un punto de entrada sencillo (check_metrics),
reduciendo la cantidad de dependencias que debe conocer.

4. Gracias a las interfaces desacopladas: cada microservicio solo necesita enviar sus métricas a través 
de la interfaz común que consume la Facade o el Adapter. No hay que escribir lógica condicional adicional
por cada nuevo microservicio que se añada al ecosistema.

5. En el módulo principal de monitoreo. Si no se usaran patrones, ese módulo tendría sentencias if/else
para convertir manualmente la respuesta de cada API externa, llamadas directas hardcodeadas a Slack/Email,
e instanciación local de parámetros de configuración.
'''

