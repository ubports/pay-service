
#include <list>
#include <string>
#include <memory>
#include <map>

#ifndef ITEM_INTERFACE_HPP__
#define ITEM_INTERFACE_HPP__ 1

namespace Item {

class IItem {
	public:
		enum Status {
			UNKNOWN
		};

		virtual std::string& getId (void) = 0;
		virtual Status getStatus (void) = 0;

		typedef std::shared_ptr<IItem> Ptr;
};

class IStore {
	public:
		virtual std::list<std::string> listApplications (void) = 0;
		virtual std::shared_ptr<std::map<std::string, IItem::Ptr>> getItems (std::string& application) = 0;
		virtual IItem::Ptr getItem (std::string& application, std::string& item) = 0;

		typedef std::shared_ptr<IStore> Ptr;
};

} // namespace Item

#endif // ITEM_INTERFACE_HPP__

