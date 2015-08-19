#ifndef _DB_INTERFACE_FACTORY_HEADER_
#define _DB_INTERFACE_FACTORY_HEADER_
#include "db_interface.h"
#include "hiredis_db_interface.h"
#include "woo_db_interface.h"
#include "api_db_interface.h"
#include "mc_db_interface.h"

class DbInterfaceFactory{

	public:
		DbInterfaceFactory(){}
		~DbInterfaceFactory(){}

	public:
		DbInterface* get_db_interface(const Db_Info_Struct& db_info_struct){
			DbInterface* p_db_interface = NULL;
			switch(db_info_struct.db_type_){
				case REDIS_DB_TYPE:
					p_db_interface = new HiRedisDbInterface(db_info_struct);
					break;
				case WOO_DB_TYPE:
					p_db_interface = new WooDbInterface(db_info_struct);
					break;
				case API_DB_TYPE:
					p_db_interface = new ApiDbInterface(db_info_struct);
					break;
				case MC_DB_TYPE:
					p_db_interface = new McDbInterface(db_info_struct);
					break;
				default:
					break;	
			}
			return p_db_interface;
		}
};

#endif
