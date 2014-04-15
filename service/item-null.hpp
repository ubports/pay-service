
class ItemNull : public Item {
		std::string _id;

	public:
		ItemNull (std::string id) : _id(id) {};

		std::string getId (void) {
			return _id;
		}

		std::string getStatus (void) {
			return Item::Status::UNKNOWN;
		}
}

class ItemDBNull : public ItemDB {
	public:
		std::list<std::string> listApplications (void) {
			return std::list<std::string>();
		}

		std::list<Item> getItems (std::string application) {
			return std::list<Item>();
		}

		Item newItem (std::string application, std::string itemid) {
			return new ItemNull(itemid);
		}
}
