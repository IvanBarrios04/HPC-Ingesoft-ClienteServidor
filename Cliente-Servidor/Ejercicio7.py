"""
Ejercicio 7: Sistema de Inventario Inteligente
Patrones: Singleton, Factory Method, Repository, Strategy, Adapter, Observer, Facade.

Flujo: Cliente -> InventoryFacade -> InventoryManager (Subject) -> Repository / Strategy / Supplier
"""
from abc import ABC, abstractmethod


# ==========================================================
# SINGLETON - Configuración centralizada
# ==========================================================
class InventoryConfig:
    """Única fuente de verdad de los parámetros críticos (umbral mínimo de stock)."""
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._min_stock_threshold = 10
        return cls._instance

    @property
    def min_stock_threshold(self):
        return self._min_stock_threshold

    @min_stock_threshold.setter
    def min_stock_threshold(self, value):
        if not isinstance(value, int) or value < 0:
            raise ValueError("El umbral debe ser un entero >= 0")
        self._min_stock_threshold = value        # cambio visible al instante para todos los módulos

    @classmethod
    def reset(cls):
        """Solo para pruebas unitarias."""
        cls._instance = None


# ==========================================================
# DOMINIO - Productos
# ==========================================================
class Product(ABC):
    category = None                               # cada subclase fija su categoría

    def __init__(self, product_id, name, stock=0, sales_history=None):
        self.product_id = product_id
        self.name = name
        self.stock = stock
        self.sales_history = sales_history if sales_history is not None else []  # unidades por periodo
        self.pending_order = False                # evita órdenes duplicadas

    @abstractmethod
    def storage_requirements(self):
        ...

    def __repr__(self):
        return f"{type(self).__name__}(id={self.product_id!r}, name={self.name!r}, stock={self.stock})"


class ElectronicProduct(Product):
    category = "electronica"

    def __init__(self, product_id, name, stock=0, sales_history=None, warranty_months=12):
        super().__init__(product_id, name, stock, sales_history)
        self.warranty_months = warranty_months

    def storage_requirements(self):
        return "Ambiente seco, protección antiestática"


class FoodProduct(Product):
    category = "alimentos"

    def storage_requirements(self):
        return "Lugar fresco y seco"


class ClothingProduct(Product):
    category = "ropa"

    def __init__(self, product_id, name, stock=0, sales_history=None, size="M"):
        super().__init__(product_id, name, stock, sales_history)
        self.size = size

    def storage_requirements(self):
        return "Almacenamiento colgado o doblado, sin humedad"


class PerishableProduct(Product):
    category = "perecederos"

    def __init__(self, product_id, name, stock=0, sales_history=None, expiration_date=None):
        super().__init__(product_id, name, stock, sales_history)
        self.expiration_date = expiration_date    # texto "AAAA-MM-DD"

    def storage_requirements(self):
        return "Refrigeración 2-8 °C, rotación FEFO"


class FrozenProduct(Product):
    category = "congelados"

    def __init__(self, product_id, name, stock=0, sales_history=None, storage_temp_c=-18.0):
        super().__init__(product_id, name, stock, sales_history)
        self.storage_temp_c = storage_temp_c

    def storage_requirements(self):
        return f"Cadena de frío continua a {self.storage_temp_c} °C"


# ==========================================================
# FACTORY METHOD - Creación de productos por categoría
# ==========================================================
class ProductFactory(ABC):
    """Creator: cada fábrica concreta decide qué Product instanciar."""
    _registry = {}

    @abstractmethod
    def create_product(self, product_id, name, stock=0, **attrs):
        ...

    @classmethod
    def register(cls, category, factory):
        cls._registry[category.lower()] = factory  # nueva categoría = nueva fábrica, sin tocar al cliente

    @classmethod
    def create(cls, category, product_id, name, stock=0, **attrs):
        try:
            factory = cls._registry[category.lower()]
        except KeyError:
            raise ValueError(f"Categoría no soportada: '{category}'. Disponibles: {sorted(cls._registry)}")
        return factory.create_product(product_id, name, stock, **attrs)


class ElectronicProductFactory(ProductFactory):
    def create_product(self, product_id, name, stock=0, **attrs):
        return ElectronicProduct(product_id, name, stock, **attrs)


class FoodProductFactory(ProductFactory):
    def create_product(self, product_id, name, stock=0, **attrs):
        return FoodProduct(product_id, name, stock, **attrs)


class ClothingProductFactory(ProductFactory):
    def create_product(self, product_id, name, stock=0, **attrs):
        return ClothingProduct(product_id, name, stock, **attrs)


class PerishableProductFactory(ProductFactory):
    def create_product(self, product_id, name, stock=0, **attrs):
        return PerishableProduct(product_id, name, stock, **attrs)


class FrozenProductFactory(ProductFactory):
    def create_product(self, product_id, name, stock=0, **attrs):
        return FrozenProduct(product_id, name, stock, **attrs)


ProductFactory.register(ElectronicProduct.category, ElectronicProductFactory())
ProductFactory.register(FoodProduct.category, FoodProductFactory())
ProductFactory.register(ClothingProduct.category, ClothingProductFactory())
ProductFactory.register(PerishableProduct.category, PerishableProductFactory())
ProductFactory.register(FrozenProduct.category, FrozenProductFactory())


# ==========================================================
# REPOSITORY - Abstracción de la persistencia
# ==========================================================
class ProductRepository(ABC):
    @abstractmethod
    def add(self, product):
        ...

    @abstractmethod
    def get(self, product_id):
        ...

    @abstractmethod
    def update(self, product):
        ...

    @abstractmethod
    def delete(self, product_id):
        ...

    @abstractmethod
    def list_all(self):
        ...


class InMemoryProductRepository(ProductRepository):
    """Migrable a SQL/ERP implementando ProductRepository, sin tocar el dominio."""

    def __init__(self):
        self._store = {}

    def add(self, product):
        if product.product_id in self._store:
            raise ValueError(f"El producto '{product.product_id}' ya existe")
        self._store[product.product_id] = product

    def get(self, product_id):
        return self._store.get(product_id)

    def update(self, product):
        if product.product_id not in self._store:
            raise KeyError(f"Producto inexistente: {product.product_id}")
        self._store[product.product_id] = product

    def delete(self, product_id):
        if product_id not in self._store:
            raise KeyError(f"Producto inexistente: {product_id}")
        del self._store[product_id]

    def list_all(self):
        return list(self._store.values())


# ==========================================================
# STRATEGY - Algoritmos de reabastecimiento
# ==========================================================
def _ceil(x):
    """Techo sin librerías externas."""
    n = int(x)
    return n + 1 if x > n else n


class ReorderContext:
    """Contexto que recibe toda estrategia: permite añadir algoritmos (p. ej. ML) sin cambiar la interfaz."""

    def __init__(self, product, threshold, month):
        self.product = product
        self.threshold = threshold
        self.month = month                        # 1-12


class ReorderStrategy(ABC):
    @abstractmethod
    def calculate_order_quantity(self, ctx):
        ...


class FixedReorderStrategy(ReorderStrategy):
    def __init__(self, quantity=50):
        self.quantity = quantity

    def calculate_order_quantity(self, ctx):
        return self.quantity


class DemandBasedReorderStrategy(ReorderStrategy):
    """Cubre `coverage_periods` de demanda promedio más el stock de seguridad (umbral)."""

    def __init__(self, coverage_periods=2, window=6, fallback=50):
        self.coverage_periods = coverage_periods
        self.window = window
        self.fallback = fallback

    def calculate_order_quantity(self, ctx):
        history = ctx.product.sales_history[-self.window:]
        if not history:
            return self.fallback
        average = sum(history) / len(history)
        target = _ceil(average * self.coverage_periods) + ctx.threshold
        return max(1, target - ctx.product.stock)


class SeasonalReorderStrategy(ReorderStrategy):
    """Decora una estrategia base amplificándola en meses de alta temporada."""

    def __init__(self, base=None, peak_months=(11, 12), multiplier=2.4):
        self.base = base if base is not None else FixedReorderStrategy(50)
        self.peak_months = peak_months
        self.multiplier = multiplier

    def calculate_order_quantity(self, ctx):
        qty = self.base.calculate_order_quantity(ctx)
        return round(qty * self.multiplier) if ctx.month in self.peak_months else qty


# ==========================================================
# ADAPTER - Proveedores externos heterogéneos
# ==========================================================
class SupplierInterface(ABC):
    """Interfaz Target uniforme que consume el motor de inventario."""

    @abstractmethod
    def order_product(self, product_id, amount):
        """Retorna el identificador de confirmación del proveedor."""


class ExternalSupplierAPI:
    """API de tercero no modificable #1 (posicional, retorna str)."""

    def send_purchase_order(self, item_code, units):
        print(f"[PROVEEDOR EXTERNO] Orden confirmada para producto {item_code} | Cantidad: {units}")
        return f"EXT-{item_code}-{units}"


class GlobalLogisticsAPI:
    """API de tercero no modificable #2 (payload dict, respuesta dict)."""
    _counter = 0

    def create_order(self, payload):
        GlobalLogisticsAPI._counter += 1
        order_id = f"GL-{GlobalLogisticsAPI._counter:04d}"
        print(f"[GLOBAL LOGISTICS] {order_id}: sku={payload['sku']} quantity={payload['quantity']}")
        return {"status": "ACCEPTED", "order_id": order_id}


class ExternalSupplierAdapter(SupplierInterface):
    def __init__(self, api):
        self._api = api

    def order_product(self, product_id, amount):
        return self._api.send_purchase_order(product_id, amount)


class GlobalLogisticsAdapter(SupplierInterface):
    def __init__(self, api):
        self._api = api

    def order_product(self, product_id, amount):
        response = self._api.create_order({"sku": product_id, "quantity": amount})
        if response["status"] != "ACCEPTED":
            raise RuntimeError(f"Proveedor rechazó la orden: {response}")
        return response["order_id"]


# ==========================================================
# OBSERVER - Eventos y alertas desacopladas
# ==========================================================
class StockEvent:
    LOW_STOCK = "LOW_STOCK"
    REORDER_PLACED = "REORDER_PLACED"

    def __init__(self, event_type, product_id, message):
        self.event_type = event_type
        self.product_id = product_id
        self.message = message


class StockObserver(ABC):
    @abstractmethod
    def update(self, event):
        ...


class EmailAlert(StockObserver):
    def __init__(self, recipient):
        self.recipient = recipient

    def update(self, event):
        print(f"[EMAIL -> {self.recipient}] {event.product_id} ({event.event_type}): {event.message}")


class SMSAlert(StockObserver):
    def __init__(self, phone):
        self.phone = phone

    def update(self, event):
        print(f"[SMS -> {self.phone}] {event.product_id} ({event.event_type}): {event.message}")


class SlackAlert(StockObserver):
    """Nuevo canal añadido sin modificar InventoryManager."""

    def __init__(self, channel):
        self.channel = channel

    def update(self, event):
        print(f"[SLACK {self.channel}] {event.product_id} ({event.event_type}): {event.message}")


class StockEventPublisher:
    """Subject reutilizable: la mecánica de Observer vive aquí, no en InventoryManager."""

    def __init__(self):
        self._observers = []

    def attach(self, observer):
        if observer not in self._observers:
            self._observers.append(observer)

    def detach(self, observer):
        self._observers.remove(observer)

    def notify(self, event):
        for obs in self._observers:
            try:
                obs.update(event)
            except Exception as exc:              # un canal caído no debe frenar a los demás
                print(f"[WARN] Falló {type(obs).__name__}: {exc}")


# ==========================================================
# INVENTORY MANAGER - Núcleo estable (no cambia al extender)
# ==========================================================
class ReorderResult:
    def __init__(self, product_id, quantity, confirmation, strategy, supplier):
        self.product_id = product_id
        self.quantity = quantity
        self.confirmation = confirmation
        self.strategy = strategy
        self.supplier = supplier

    def __repr__(self):
        return (f"ReorderResult(product_id={self.product_id!r}, quantity={self.quantity}, "
                f"confirmation={self.confirmation!r}, strategy={self.strategy!r}, "
                f"supplier={self.supplier!r})")


class InventoryManager:
    def __init__(self, repository, default_supplier, default_strategy, config):
        self._repo = repository
        self._config = config
        self._default_supplier = default_supplier
        self._default_strategy = default_strategy
        self._strategies = {}                     # overrides por categoría
        self._suppliers = {}                      # overrides por categoría
        self._publisher = StockEventPublisher()
        self._current_month = 1                   # 1-12; configurable (define la temporada)

    # ---- Observer ----
    def attach(self, observer):
        self._publisher.attach(observer)

    def detach(self, observer):
        self._publisher.detach(observer)

    # ---- Strategy / Adapter: configuración en runtime ----
    def set_strategy(self, strategy, category=None):
        if category is None:
            self._default_strategy = strategy
        else:
            self._strategies[category.lower()] = strategy

    def set_supplier(self, supplier, category=None):
        if category is None:
            self._default_supplier = supplier
        else:
            self._suppliers[category.lower()] = supplier

    def set_current_month(self, month):
        if not isinstance(month, int) or not 1 <= month <= 12:
            raise ValueError("El mes debe ser un entero entre 1 y 12")
        self._current_month = month

    # ---- Operaciones de stock ----
    def record_sale(self, product_id, units):
        product = self._require(product_id)
        if units <= 0 or units > product.stock:
            raise ValueError(f"Venta inválida ({units}) con stock {product.stock}")
        product.stock -= units
        product.sales_history.append(units)
        self._repo.update(product)

    def receive_shipment(self, product_id, units):
        product = self._require(product_id)
        if units <= 0:
            raise ValueError("La cantidad recibida debe ser positiva")
        product.stock += units
        product.pending_order = False             # ciclo cerrado: puede volver a alertar
        self._repo.update(product)

    # ---- Monitoreo y reorden ----
    def monitor_inventory(self):
        results = []
        for product in self._repo.list_all():
            result = self.check_product(product)
            if result is not None:
                results.append(result)
        return results

    def check_product(self, product):
        threshold = self._config.min_stock_threshold
        if product.stock > threshold or product.pending_order:
            return None

        self._publisher.notify(StockEvent(
            StockEvent.LOW_STOCK, product.product_id,
            f"'{product.name}' con stock crítico: {product.stock} (umbral {threshold})"))

        strategy = self._strategies.get(product.category, self._default_strategy)
        supplier = self._suppliers.get(product.category, self._default_supplier)
        qty = strategy.calculate_order_quantity(ReorderContext(product, threshold, self._current_month))
        if qty <= 0:
            return None

        print(f"Generando orden automática de '{product.name}' ({type(strategy).__name__})...")
        confirmation = supplier.order_product(product.product_id, qty)
        product.pending_order = True
        self._repo.update(product)
        self._publisher.notify(StockEvent(
            StockEvent.REORDER_PLACED, product.product_id,
            f"Orden {confirmation} por {qty} uds de '{product.name}'"))
        return ReorderResult(product.product_id, qty, confirmation,
                             type(strategy).__name__, type(supplier).__name__)

    def stock_report(self):
        return [(p.product_id, p.name, p.stock, p.pending_order) for p in self._repo.list_all()]

    def _require(self, product_id):
        product = self._repo.get(product_id)
        if product is None:
            raise KeyError(f"Producto inexistente: {product_id}")
        return product


# ==========================================================
# FACADE - Punto de entrada único
# ==========================================================
class InventoryFacade:
    """Solo orquesta y simplifica; la lógica vive en InventoryManager, Repository, etc."""

    def __init__(self, repository=None, supplier=None, strategy=None, config=None):
        self._config = config if config is not None else InventoryConfig()
        self._repo = repository if repository is not None else InMemoryProductRepository()
        self._manager = InventoryManager(
            self._repo,
            supplier if supplier is not None else ExternalSupplierAdapter(ExternalSupplierAPI()),
            strategy if strategy is not None else FixedReorderStrategy(),
            self._config,
        )

    def register_product(self, category, product_id, name, stock=0, **attrs):
        if stock < 0:
            raise ValueError("El stock inicial no puede ser negativo")
        product = ProductFactory.create(category, product_id, name, stock, **attrs)
        self._repo.add(product)
        return product

    def set_min_stock_threshold(self, value):
        self._config.min_stock_threshold = value

    def set_reorder_strategy(self, strategy, category=None):
        self._manager.set_strategy(strategy, category)

    def set_supplier(self, supplier, category=None):
        self._manager.set_supplier(supplier, category)

    def set_current_month(self, month):
        self._manager.set_current_month(month)

    def add_alert_channel(self, observer):
        self._manager.attach(observer)

    def remove_alert_channel(self, observer):
        self._manager.detach(observer)

    def record_sale(self, product_id, units):
        self._manager.record_sale(product_id, units)

    def receive_shipment(self, product_id, units):
        self._manager.receive_shipment(product_id, units)

    def monitor_inventory(self):
        return self._manager.monitor_inventory()

    def stock_report(self):
        return self._manager.stock_report()


# ==========================================================
# EJEMPLO DE USO
# ==========================================================
if __name__ == "__main__":
    print("=== Singleton: una sola configuración compartida ===")
    print("InventoryConfig() is InventoryConfig():", InventoryConfig() is InventoryConfig())

    print("\n=== Configuración del sistema ===")
    inventario = InventoryFacade()
    inventario.set_current_month(12)              # diciembre: activa la estrategia estacional

    email, sms, slack = EmailAlert("logistica@empresa.com"), SMSAlert("+57 300 000 0000"), SlackAlert("#inventario")
    for canal in (email, sms, slack):
        inventario.add_alert_channel(canal)

    inventario.set_reorder_strategy(SeasonalReorderStrategy(FixedReorderStrategy(50)), "electronica")
    inventario.set_reorder_strategy(DemandBasedReorderStrategy(), "perecederos")
    inventario.set_supplier(GlobalLogisticsAdapter(GlobalLogisticsAPI()), "perecederos")

    inventario.register_product("electronica", "PROD-102", "Teclado Mecánico", stock=5)
    inventario.register_product("perecederos", "PROD-201", "Yogurt", stock=8, sales_history=[30, 28, 35, 32])
    inventario.register_product("ropa", "PROD-301", "Camiseta", stock=50, size="L")

    print("\n=== Monitoreo 1: alertas + órdenes automáticas ===")
    for r in inventario.monitor_inventory():
        print("  ->", r)

    print("\n=== Monitoreo 2: sin duplicados (hay órdenes pendientes) ===")
    print("  ->", inventario.monitor_inventory())

    print("\n=== Cambios en runtime: llega pedido, sube el umbral y se retira Slack ===")
    inventario.remove_alert_channel(slack)
    inventario.receive_shipment("PROD-102", 120)
    inventario.set_min_stock_threshold(60)
    for r in inventario.monitor_inventory():
        print("  ->", r)

    print("\n=== Reporte ===")
    for row in inventario.stock_report():
        print(" ", row)

    print("\n=== Validaciones ===")
    for label, action in [
        ("Categoría inexistente", lambda: inventario.register_product("juguetes", "X-1", "Robot")),
        ("Stock inicial negativo", lambda: inventario.register_product("ropa", "X-2", "Gorra", stock=-1)),
        ("Producto duplicado", lambda: inventario.register_product("ropa", "PROD-301", "Otra")),
        ("Venta mayor al stock", lambda: inventario.record_sale("PROD-301", 999)),
        ("Producto inexistente", lambda: inventario.receive_shipment("NO-EXISTE", 5)),
        ("Umbral inválido", lambda: inventario.set_min_stock_threshold(-3)),
        ("Mes inválido", lambda: inventario.set_current_month(13)),
    ]:
        try:
            action()
        except (ValueError, KeyError) as exc:
            print(f"{label}: {exc.args[0]}")


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
