/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2018 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cellapp.h"
#include "witness.h"
#include "entityref.h"
#include "client_entity.h"
#include "client_entity_method.h"
#include "helper/debug_helper.h"
#include "network/packet.h"
#include "network/bundle.h"
#include "network/network_interface.h"
#include "server/components.h"
#include "client_lib/client_interface.h"
#include "entitydef/method.h"
#include "entitydef/property.h"
#include "entitydef/scriptdef_module.h"

#include "../../server/baseapp/baseapp_interface.h"
#include "../../server/cellapp/cellapp_interface.h"

namespace KBEngine{

SCRIPT_METHOD_DECLARE_BEGIN(ClientEntityComponent)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(ClientEntityComponent)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(ClientEntityComponent)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(ClientEntityComponent, 0, 0, 0, 0, 0)


//-------------------------------------------------------------------------------------
ClientEntityComponent::ClientEntityComponent(PropertyDescription* pComponentPropertyDescription, ClientEntity* pClientEntity):
	ScriptObject(getScriptType(), false),
	pClientEntity_(pClientEntity),
	pComponentPropertyDescription_(pComponentPropertyDescription)
{
	Py_INCREF(pClientEntity_);
}

//-------------------------------------------------------------------------------------
ClientEntityComponent::~ClientEntityComponent()
{
	Py_DECREF(pClientEntity_);
}

//-------------------------------------------------------------------------------------
ScriptDefModule* ClientEntityComponent::pComponentScriptDefModule()
{
	EntityComponentType* pEntityComponentType = static_cast<EntityComponentType*>(pComponentPropertyDescription_->getDataType());
	return pEntityComponentType->pScriptDefModule();
}

//-------------------------------------------------------------------------------------
PyObject* ClientEntityComponent::onScriptGetAttribute(PyObject* attr)
{
	Entity* srcEntity = Cellapp::getSingleton().findEntity(pClientEntity_->srcEntityID());

	if (srcEntity == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "Entity::ClientEntityComponent: srcEntityID(%d) not found!\n",
			pClientEntity_->srcEntityID());
		PyErr_PrintEx(0);
		return 0;
	}

	if (srcEntity->isDestroyed())
	{
		PyErr_Format(PyExc_AssertionError, "Entity::ClientEntityComponent: srcEntityID(%d) is destroyed!\n",
			pClientEntity_->srcEntityID());
		PyErr_PrintEx(0);
		return 0;
	}

	if (srcEntity->pWitness() == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "%s::ClientEntityComponent: no client, srcEntityID(%d).\n",
			srcEntity->scriptName(), srcEntity->id());
		PyErr_PrintEx(0);
		return 0;
	}

	EntityRef* pEntityRef = srcEntity->pWitness()->getViewEntityRef(pClientEntity_->clientEntityID());
	Entity* e = (pEntityRef && ((pEntityRef->flags() & ENTITYREF_FLAG_ENTER_CLIENT_PENDING) <= 0))
		? pEntityRef->pEntity() : NULL;

	if (e == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "%s::ClientEntityComponent: not found entity(%d), srcEntityID(%d).\n",
			srcEntity->scriptName(), pClientEntity_->clientEntityID(), srcEntity->id());
		PyErr_PrintEx(0);
		return 0;
	}

	wchar_t* PyUnicode_AsWideCharStringRet0 = PyUnicode_AsWideCharString(attr, NULL);
	char* ccattr = strutil::wchar2char(PyUnicode_AsWideCharStringRet0);
	PyMem_Free(PyUnicode_AsWideCharStringRet0);

	ScriptDefModule* pScriptDefModule = pComponentScriptDefModule();
	MethodDescription* pMethodDescription = pScriptDefModule->findClientMethodDescription(ccattr);

	free(ccattr);

	if (pMethodDescription != NULL)
	{
		return new ClientEntityMethod(pComponentPropertyDescription_, pScriptDefModule, pMethodDescription, pClientEntity_->srcEntityID(), pClientEntity_->clientEntityID());
	}

	return ScriptObject::onScriptGetAttribute(attr);
}

//-------------------------------------------------------------------------------------
PyObject* ClientEntityComponent::tp_repr()
{
	char s[1024];
	c_str(s, 1024);
	return PyUnicode_FromString(s);
}

//-------------------------------------------------------------------------------------
void ClientEntityComponent::c_str(char* s, size_t size)
{
	kbe_snprintf(s, size, "clientEntityComponent id:%d, srcEntityID=%d.", pClientEntity_->clientEntityID(), pClientEntity_->srcEntityID());
}

//-------------------------------------------------------------------------------------
PyObject* ClientEntityComponent::tp_str()
{
	return tp_repr();
}

//-------------------------------------------------------------------------------------
SCRIPT_METHOD_DECLARE_BEGIN(ClientEntity)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(ClientEntity)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(ClientEntity)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(ClientEntity, 0, 0, 0, 0, 0)		

//-------------------------------------------------------------------------------------
ClientEntity::ClientEntity(ENTITY_ID srcEntityID,
		ENTITY_ID clientEntityID):
ScriptObject(getScriptType(), false),
srcEntityID_(srcEntityID),
clientEntityID_(clientEntityID)
{
}

//-------------------------------------------------------------------------------------
ClientEntity::~ClientEntity()
{
}

//-------------------------------------------------------------------------------------
PyObject* ClientEntity::onScriptGetAttribute(PyObject* attr)
{
	Entity* srcEntity = Cellapp::getSingleton().findEntity(srcEntityID_);

	if(srcEntity == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "Entity::clientEntity: srcEntityID(%d) not found!\n",		
			 srcEntityID_);		
		PyErr_PrintEx(0);
		return 0;
	}

	if(srcEntity->isDestroyed())
	{
		PyErr_Format(PyExc_AssertionError, "Entity::clientEntity: srcEntityID(%d) is destroyed!\n",		
			srcEntityID_);		
		PyErr_PrintEx(0);
		return 0;
	}

	if(srcEntity->pWitness() == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "%s::clientEntity: no client, srcEntityID(%d).\n",		
			srcEntity->scriptName(), srcEntity->id());		
		PyErr_PrintEx(0);
		return 0;
	}

	EntityRef* pEntityRef = srcEntity->pWitness()->getViewEntityRef(clientEntityID_);
	Entity* e = (pEntityRef && ((pEntityRef->flags() & ENTITYREF_FLAG_ENTER_CLIENT_PENDING) <= 0))
		? pEntityRef->pEntity() : NULL;

	if(e == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "%s::clientEntity: not found entity(%d), srcEntityID(%d).\n",		
			srcEntity->scriptName(), clientEntityID_, srcEntity->id());		
		PyErr_PrintEx(0);
		return 0;
	}

	wchar_t* PyUnicode_AsWideCharStringRet0 = PyUnicode_AsWideCharString(attr, NULL);
	char* ccattr = strutil::wchar2char(PyUnicode_AsWideCharStringRet0);
	PyMem_Free(PyUnicode_AsWideCharStringRet0);

	MethodDescription* pMethodDescription = const_cast<ScriptDefModule*>(e->pScriptModule())->findClientMethodDescription(ccattr);

	if(pMethodDescription != NULL)
	{
		free(ccattr);
		return new ClientEntityMethod(NULL, e->pScriptModule(), pMethodDescription, srcEntityID_, clientEntityID_);
	}
	else
	{
		// �Ƿ��������������
		PropertyDescription* pComponentPropertyDescription = const_cast<ScriptDefModule*>(e->pScriptModule())->findComponentPropertyDescription(ccattr);
		if (pComponentPropertyDescription)
		{
			free(ccattr);
			return new ClientEntityComponent(pComponentPropertyDescription, this);
		}
	}

	free(ccattr);
	return ScriptObject::onScriptGetAttribute(attr);
}

//-------------------------------------------------------------------------------------
PyObject* ClientEntity::tp_repr()
{
	char s[1024];
	c_str(s, 1024);
	return PyUnicode_FromString(s);
}

//-------------------------------------------------------------------------------------
void ClientEntity::c_str(char* s, size_t size)
{
	kbe_snprintf(s, size, "clientEntity id:%d, srcEntityID=%d.", clientEntityID_, srcEntityID_);
}

//-------------------------------------------------------------------------------------
PyObject* ClientEntity::tp_str()
{
	return tp_repr();
}


//-------------------------------------------------------------------------------------

}

