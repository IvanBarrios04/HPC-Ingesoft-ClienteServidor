from abc import ABC, abstractmethod

# ==========================================
# SINGLETON - Configuración de Inventario
# ==========================================
class InventoryConfig:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(InventoryConfig, cls).__new__(cls)
            cls._instance.min_stock_threshold = 10
        return cls._instance


# ==========================================
# STRATEGY - Estrategias de Reabastecimiento
# ==========================================
class ReorderStrategy(ABC):
    @abstractmethod
    def calculate_order_quantity(self, current_stock: int) -> int:
        pass

class FixedReorderStrategy(ReorderStrategy):
    def calculate_order_quantity(self, current_stock: int) -> int:
        return 50  # Pedido fijo

class SeasonalReorderStrategy(ReorderStrategy):
    def calculate_order_quantity(self, current_stock: int) -> int:
        return 120  # Pedido ampliado por alta temporada


# ==========================================
# ADAPTER - Proveedores Externos
# ==========================================
class ExternalSupplierAPI:
    """API de tercero no modificable"""
    def send_purchase_order(self, item_code: str, units: int):
        print(f"[PROVEEDOR EXTERNO] Orden confirmada para Producto #{item_code} | Cantidad: {units}")

class SupplierAdapter:
    def __init__(self, external_api: ExternalSupplierAPI):
        self.external_api = external_api

    def order_product(self, product_id: str, amount: int):
        self.external_api.send_purchase_order(product_id, amount)


# ==========================================
# OBSERVER - Alertas de Stock
# ==========================================
class StockObserver(ABC):
    @abstractmethod
    def update(self, product_name: str, stock: int):
        pass

class StockAlertNotifier(StockObserver):
    def update(self, product_name: str, stock: int):
        print(f"[ALERTA STOCK] El producto '{product_name}' tiene un nivel crítico de {stock} unidades.")


# ==========================================
# FACADE - Sistema de Inventario Central
# ==========================================
class InventoryFacade:
    def __init__(self, supplier_adapter: SupplierAdapter, reorder_strategy: ReorderStrategy):
        self.config = InventoryConfig()
        self.supplier = supplier_adapter
        self.strategy = reorder_strategy
        self.observers = []

    def attach(self, observer: StockObserver):
        self.observers.append(observer)

    def notify(self, product_name: str, stock: int):
        for obs in self.observers:
            obs.update(product_name, stock)

    def check_and_reorder(self, product_id: str, product_name: str, current_stock: int):
        print(f"Verificando inventario de '{product_name}': {current_stock} unidades.")
        if current_stock <= self.config.min_stock_threshold:
            self.notify(product_name, current_stock)
            qty_to_order = self.strategy.calculate_order_quantity(current_stock)
            print(f"Generando orden de reabastecimiento automáticamente...")
            self.supplier.order_product(product_id, qty_to_order)


# ==========================================
# EJEMPLO DE USO
# ==========================================
if __name__ == "__main__":
    # Integración con proveedor externo vía Adapter
    external_api = ExternalSupplierAPI()
    adapter = SupplierAdapter(external_api)

    # Definir estrategia de reposición estacional
    estrategia = SeasonalReorderStrategy()

    # Instanciar Facade
    inventario = InventoryFacade(adapter, estrategia)
    inventario.attach(StockAlertNotifier())

    # Probar flujo con stock bajo
    inventario.check_and_reorder("PROD-102", "Teclado Mecánico", 5)


'''
PREGUNTAS

1. Implementando una nueva clase concreta de SupplierAdapter para la API del proveedor entrante. El 
sistema de inventario seguirá invocando el método estándar (por ejemplo, order_product), mientras que 
el nuevo adaptador maneja los detalles específicos del nuevo proveedor.

2. Porque las reglas comerciales de inventario cambian constantemente (reorden fijo, según demanda 
histórica o temporadas alta/baja). Al aislarlas mediante el patrón Strategy, se pueden intercambiar los
 algoritmos en tiempo de ejecución sin alterar el flujo del inventario.

3. El módulo de gestión de stock tendría que invocar directamente a cada servicio de notificación 
(Email, SMS, Slack, etc.). Esto acopla el inventario a la infraestructura de comunicación, obligando a
modificar la lógica central del stock cada vez que se agrega o quita un canal de alerta.

4. Utilizando Facade para abstraer la interacción secuencial entre la verificación de umbrales en
InventoryConfig, la lógica de ReorderStrategy, el envío mediante SupplierAdapter y la notificación a
StockObserver.

5. El patrón Facade (Fachada). Proporciona una interfaz unificada y simplificada que oculta las
complejidades de interacción de múltiples subsistemas de bajo nivel a los clientes del sistema.
'''