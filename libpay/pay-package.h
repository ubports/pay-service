

typedef struct _PayPackage PayPackage;
typedef enum {
	PAY_PACKAGE_ITEM_STATUS_UNKNOWN,
	PAY_PACKAGE_ITEM_STATUS_VERIFYING,
	PAY_PACKAGE_ITEM_STATUS_PURCHASED,
	PAY_PACKAGE_ITEM_STATUS_PURCHASING,
	PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED
} PayPackageItemStatus;
typedef void (*PayPackageItemObserver) (PayPackage * package, const char * itemid, PayPackageItemStatus status);

PayPackage * pay_package_create (const char * package_name);
void pay_package_delete (PayPackage * package);
PayPackageItemStatus pay_package_item_status (PayPackage * package, const char * itemid);
gboolean pay_package_item_observer_add (PayPackage * package, PayPackageItemObserver observer, gpointer user_data);
gboolean pay_package_item_observer_delete (PayPackage * package, PayPackageItemObserver observer, gpointer user_data);

gboolean pay_package_item_start_verification (PayPackage * package, const char * itemid);
gboolean pay_package_item_start_purchase (PayPackage * package, const char * itemid);

