from abc import ABC, abstractmethod
from datetime import date
from threading import Lock

# ==========================================================
# SINGLETON - Configuración centralizada
# ==========================================================

class InventoryConfig:
    _instance = None
    _lock = Lock()

    def __new__(cls):
        # Double-checked locking: seguro ante acceso concurrente
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    inst = super().__new__(cls)
                    inst.min_stock_threshold = 10
                    cls._instance = inst
        return cls._instance


# ==========================================================
# FACTORY METHOD - Creación de productos
# ==========================================================

class Product:
    category = "general"

    def __init__(self, sku, name, stock, demand_history=None):
        self.sku = sku
        self.name = name
        self.stock = stock
        # Unidades vendidas por período (insumo de DemandBasedReorderStrategy)
        self.demand_history = demand_history or []

    def __repr__(self):
        return f"{self.category}:{self.name}(stock={self.stock})"


class ElectronicProduct(Product):
    category = "electronica"


class FoodProduct(Product):
    category = "alimentos"


class ClothingProduct(Product):
    category = "ropa"


class ProductFactory:
    # Registro extensible: nuevas categorías se agregan sin tocar create_product (OCP)
    _registry = {
        "electronica": ElectronicProduct,
        "alimentos": FoodProduct,
        "ropa": ClothingProduct,
    }

    @classmethod
    def register_category(cls, category: str, product_cls: type):
        cls._registry[category] = product_cls

    @classmethod
    def create_product(cls, category: str, sku, name, stock, demand_history=None) -> Product:
        try:
            return cls._registry[category](sku, name, stock, demand_history)
        except KeyError:
            raise ValueError(f"Categoría no soportada: {category}")


# ==========================================================
# REPOSITORY - Persistencia abstracta
# ==========================================================

class ProductRepository(ABC):
    @abstractmethod
    def add(self, product: Product): ...

    @abstractmethod
    def get(self, sku: str) -> Product: ...

    @abstractmethod
    def get_all(self) -> list: ...

    @abstractmethod
    def update(self, product: Product): ...

    @abstractmethod
    def delete(self, sku: str): ...


class InMemoryProductRepository(ProductRepository):
    def __init__(self):
        self._store = {}

    def add(self, product):
        if product.sku in self._store:
            raise ValueError(f"SKU duplicado: {product.sku}")
        self._store[product.sku] = product

    def get(self, sku):
        return self._store.get(sku)

    def get_all(self):
        return list(self._store.values())

    def update(self, product):
        if product.sku not in self._store:
            raise KeyError(product.sku)
        self._store[product.sku] = product

    def delete(self, sku):
        self._store.pop(sku, None)


# ==========================================================
# STRATEGY - Reposición
# ==========================================================

class ReorderStrategy(ABC):
    @abstractmethod
    def calculate_quantity(self, product: Product) -> int:
        pass


class FixedReorderStrategy(ReorderStrategy):
    def __init__(self, quantity=50):
        self.quantity = quantity

    def calculate_quantity(self, product):
        return self.quantity


class DemandBasedReorderStrategy(ReorderStrategy):
    def __init__(self, periods_cover=2):
        self.periods_cover = periods_cover

    def calculate_quantity(self, product):
        if not product.demand_history:
            return 0
        avg = sum(product.demand_history) / len(product.demand_history)
        return max(0, round(avg * self.periods_cover) - product.stock)


class SeasonalReorderStrategy(ReorderStrategy):
    def __init__(self, base=50, factors=None):
        self.base = base
        self.factors = factors or {11: 1.5, 12: 2.0}  # temporada alta

    def calculate_quantity(self, product):
        return round(self.base * self.factors.get(date.today().month, 1.0))


# ==========================================================
# ADAPTER - Proveedores externos (APIs no modificables)
# ==========================================================

class ExternalSupplierAPI:
    """API de terceros: interfaz propia e incompatible."""
    def send_purchase_order(self, item_code, units):
        return f"[ExternalSupplierAPI] PO item={item_code} units={units}"


class LegacySupplierAPI:
    """Otro proveedor, otro protocolo (payload tipo dict)."""
    def createPO(self, payload: dict):
        return f"[LegacySupplierAPI] PO {payload}"


class SupplierAdapter(ABC):
    @abstractmethod
    def order(self, product: Product, quantity: int) -> str:
        pass


class ExternalSupplierAdapter(SupplierAdapter):
    def __init__(self, api: ExternalSupplierAPI):
        self.api = api

    def order(self, product, quantity):
        return self.api.send_purchase_order(product.sku, quantity)


class LegacySupplierAdapter(SupplierAdapter):
    def __init__(self, api: LegacySupplierAPI):
        self.api = api

    def order(self, product, quantity):
        return self.api.createPO({"sku": product.sku, "qty": quantity})


# ==========================================================
# OBSERVER - Alertas de stock
# ==========================================================

class StockObserver(ABC):
    @abstractmethod
    def update(self, message):
        pass


class EmailAlert(StockObserver):
    def update(self, message):
        print(f"[EMAIL] {message}")


class SMSAlert(StockObserver):
    def update(self, message):
        print(f"[SMS] {message}")


# ==========================================================
# INVENTORY MANAGER (Subject + orquestación de reposición)
# ==========================================================

class InventoryManager:
    def __init__(self, repository: ProductRepository, supplier: SupplierAdapter):
        self.repository = repository
        self.supplier = supplier
        self.config = InventoryConfig()
        self.observers = []
        self.default_strategy: ReorderStrategy = FixedReorderStrategy()
        self.category_strategies = {}

    # Observer
    def attach(self, observer: StockObserver):
        self.observers.append(observer)

    def notify(self, message):
        for obs in self.observers:
            obs.update(message)

    # Strategy (por categoría, intercambiable en runtime)
    def set_strategy(self, category: str, strategy: ReorderStrategy):
        self.category_strategies[category] = strategy

    def _strategy_for(self, product):
        return self.category_strategies.get(product.category, self.default_strategy)

    # Monitoreo
    def check_stock(self):
        orders = []
        for product in self.repository.get_all():
            if product.stock < self.config.min_stock_threshold:
                self.notify(
                    f"Stock bajo: {product.name} ({product.stock} < "
                    f"{self.config.min_stock_threshold})"
                )
                qty = self._strategy_for(product).calculate_quantity(product)
                if qty > 0:
                    orders.append(self.supplier.order(product, qty))
        return orders


# ==========================================================
# FACADE - Interfaz simplificada
# ==========================================================

class InventoryFacade:
    def __init__(self, supplier: SupplierAdapter = None):
        supplier = supplier or ExternalSupplierAdapter(ExternalSupplierAPI())
        self._repo = InMemoryProductRepository()
        self._manager = InventoryManager(self._repo, supplier)
        self._config = InventoryConfig()

    def register_product(self, category, sku, name, stock, demand_history=None):
        product = ProductFactory.create_product(category, sku, name, stock, demand_history)
        self._repo.add(product)
        return product

    def set_threshold(self, threshold: int):
        self._config.min_stock_threshold = threshold

    def set_reorder_strategy(self, category, strategy: ReorderStrategy):
        self._manager.set_strategy(category, strategy)

    def subscribe_alert(self, observer: StockObserver):
        self._manager.attach(observer)

    def update_stock(self, sku, new_stock):
        product = self._repo.get(sku)
        product.stock = new_stock
        self._repo.update(product)

    def monitor_inventory(self):
        orders = self._manager.check_stock()
        for o in orders:
            print(f"[ORDEN] {o}")
        return orders


# ==========================================================
# EJEMPLO DE USO
# ==========================================================

if __name__ == "__main__":
    inventory = InventoryFacade()

    # Alertas desacopladas
    inventory.subscribe_alert(EmailAlert())
    inventory.subscribe_alert(SMSAlert())

    # Productos (Factory + Repository)
    inventory.register_product("electronica", "E-001", "Laptop", 4)
    inventory.register_product("alimentos", "A-001", "Arroz", 3, demand_history=[40, 60, 50])
    inventory.register_product("ropa", "R-001", "Camiseta", 25)

    # Estrategias por categoría
    inventory.set_reorder_strategy("alimentos", DemandBasedReorderStrategy())
    inventory.set_reorder_strategy("electronica", SeasonalReorderStrategy(base=20))

    # Singleton: cualquier módulo ve el mismo umbral
    assert InventoryConfig() is InventoryConfig()
    print(f"Umbral: {InventoryConfig().min_stock_threshold}\n")

    inventory.monitor_inventory()

    # Cambio de proveedor sin tocar el núcleo
    print("\n--- Cambio de proveedor (Adapter) ---")
    inventory2 = InventoryFacade(LegacySupplierAdapter(LegacySupplierAPI()))
    inventory2.subscribe_alert(EmailAlert())
    inventory2.register_product("ropa", "R-002", "Chaqueta", 2)
    inventory2.monitor_inventory()
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
