/* -----------------------------------------------------
This source code is public domain ( CC0 )
The code is provided as-is without limitations, requirements and responsibilities.
Creators and contributors to this source code are provided as a token of appreciation
and no one associated with this source code can be held responsible for any possible
damages or losses of any kind.

Original file creator:  Niko Kauppi (Code maintenance)
Contributors:
----------------------------------------------------- */
/*
        Copyright 2021 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Renderer.h"
#include "Shared.h"
#include "Window.h"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>

#include "VulkanTools.h"
#include <inttypes.h>
#include "object_type_string_helper.h"

#define _DEBUG_ (1)

VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData) {

  std::ostringstream stream;
  stream << "VKDBG: ";

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    stream << "VERBOSE : ";
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    stream << "INFO : ";
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    stream << "WARNING : ";
  }
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    stream << "ERROR : ";
  }

  if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
    stream << "GENERAL";
  }
  else {
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
      stream << "VALIDATION";
      //validation_error = 1;
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
      if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        stream << "|";
      }
      stream << "PERFORMANCE";
    }
  }


  stream << "@[" << pCallbackData->messageIdNumber << " " << pCallbackData->pMessageIdName << "]: ";
  stream << pCallbackData->pMessage << std::endl;

  if (pCallbackData->objectCount > 0) {
    char tmp_message[500];
    sprintf(tmp_message, "\n\tObjects - %d\n", pCallbackData->objectCount);
    stream <<  tmp_message << std::endl;
    for (uint32_t object = 0; object < pCallbackData->objectCount; ++object) {
      sprintf(tmp_message, "\t\tObject[%d] - %s", object, string_VkObjectType(pCallbackData->pObjects[object].objectType));
      stream << tmp_message;

      VkObjectType t = pCallbackData->pObjects[object].objectType;
      if (t == VK_OBJECT_TYPE_INSTANCE || t == VK_OBJECT_TYPE_PHYSICAL_DEVICE || t == VK_OBJECT_TYPE_DEVICE ||
        t == VK_OBJECT_TYPE_COMMAND_BUFFER || t == VK_OBJECT_TYPE_QUEUE) {
        sprintf(tmp_message, ", Handle %p", (void *)(uintptr_t)(pCallbackData->pObjects[object].objectHandle));
        stream << tmp_message;
      }
      else {
        sprintf(tmp_message, ", Handle Ox%" PRIx64, (pCallbackData->pObjects[object].objectHandle));
        stream << tmp_message;
      }

      if (NULL != pCallbackData->pObjects[object].pObjectName && strlen(pCallbackData->pObjects[object].pObjectName) > 0) {
        sprintf(tmp_message, ", Name \"%s\"", pCallbackData->pObjects[object].pObjectName);
        stream << tmp_message;
      }
      
      stream <<  tmp_message << std::endl;
    }
  }


  if (pCallbackData->cmdBufLabelCount > 0) {
    char tmp_message[500];
    sprintf(tmp_message, "\n\tCommand Buffer Labels - %d\n", pCallbackData->cmdBufLabelCount);
    //strcat(message, tmp_message);
    stream << tmp_message;
    for (uint32_t cmd_buf_label = 0; cmd_buf_label < pCallbackData->cmdBufLabelCount; ++cmd_buf_label) {
      sprintf(tmp_message, "\t\tLabel[%d] - %s { %f, %f, %f, %f}\n", cmd_buf_label,
        pCallbackData->pCmdBufLabels[cmd_buf_label].pLabelName, pCallbackData->pCmdBufLabels[cmd_buf_label].color[0],
        pCallbackData->pCmdBufLabels[cmd_buf_label].color[1], pCallbackData->pCmdBufLabels[cmd_buf_label].color[2],
        pCallbackData->pCmdBufLabels[cmd_buf_label].color[3]);
      stream << tmp_message;
    }
  }


  std::cout << stream.str();
  std::string debugMessage = stream.str();
  OutputDebugStringA(debugMessage.c_str());

#if defined(ANDROID)
  LOGE("%s", stream.str().c_str());
#endif

//#if defined( _WIN32 )
//  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
//    //MessageBox( NULL, stream.str().c_str(), _T("Vulkan Error!"), 0 );
//  }
//#endif
  return false;
}

Renderer::Renderer()
{
  LOGI("InitPlatform in");
  InitPlatform();
  LOGI("InitPlatform out");
  _SetupLayersAndExtensions();
#if _DEBUG_
  _SetupDebug();
#endif
  LOGI("_InitInstance in");
  _InitInstance();
  LOGI("_InitInstance out");
#if _DEBUG_
  _InitDebug();
#endif
  LOGI("_InitDevice in");
  _InitDevice();
  LOGI("_InitDevice out");
}

Renderer::~Renderer()
{
  delete _window;

  _DeInitDevice();
#if _DEBUG_
  _DeInitDebug();
#endif
  _DeInitInstance();
  DeInitPlatform();
}

void Renderer::setNativeWindow(void * nativeWindow) {
  if (_window == NULL) {
#if defined(ANDROID)
    LOGI("%s", "setNativeWindow first");
#endif
    OpenWindow(800, 600, "Yaba sanshiro Vulkan", nativeWindow);
  }
  else {
#if defined(ANDROID)
    LOGI("%s", "setNativeWindow 2");
#endif
    _window->setNativeWindow(nativeWindow);
  }

}

Window * Renderer::OpenWindow(uint32_t size_x, uint32_t size_y, std::string name, void * nativeWindow)
{
  _window = new Window(this, size_x, size_y, name, nativeWindow);
  return		_window;
}

bool Renderer::Run()
{
  if (nullptr != _window) {
    return _window->Update();
  }
  return true;
}

const VkInstance Renderer::GetVulkanInstance() const
{
  return _instance;
}

const VkPhysicalDevice Renderer::GetVulkanPhysicalDevice() const
{
  return _gpu;
}

const VkDevice Renderer::GetVulkanDevice() const
{
  return _device;
}

const VkQueue Renderer::GetVulkanQueue() const
{
  return _queue;
}

const VkQueue Renderer::GetComputeQueue() const
{
  return _queueCompute;
}


const uint32_t Renderer::GetVulkanGraphicsQueueFamilyIndex() const
{
  return _graphics_family_index;
}

const uint32_t Renderer::GetVulkanComputeQueueFamilyIndex() const
{
  return _compute_family_index;
}


const VkPhysicalDeviceProperties & Renderer::GetVulkanPhysicalDeviceProperties() const
{
  return _gpu_properties;
}

const VkPhysicalDeviceMemoryProperties & Renderer::GetVulkanPhysicalDeviceMemoryProperties() const
{
  return _gpu_memory_properties;
}

void Renderer::_SetupLayersAndExtensions()
{
  _instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  AddRequiredPlatformInstanceExtensions(&_instance_extensions);

  _device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  _device_extensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
  AddRequiredPlatformDeviceExtensions(&_device_extensions);
}

void Renderer::_InitInstance()
{
  VkApplicationInfo application_info{};
  application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  application_info.pApplicationName = "YABASANSHIRO";
  application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.pEngineName = "YABASANSHIRO";
  application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.apiVersion = VK_API_VERSION_1_1;

 
  VkInstanceCreateInfo instance_create_info{};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &application_info;
  instance_create_info.enabledLayerCount = _instance_layers.size();
  instance_create_info.ppEnabledLayerNames = _instance_layers.data();
  instance_create_info.enabledExtensionCount = _instance_extensions.size();
  instance_create_info.ppEnabledExtensionNames = _instance_extensions.data();

#if _DEBUG_
  dbg_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  dbg_messenger_create_info.pNext = NULL;
  dbg_messenger_create_info.flags = 0;
  dbg_messenger_create_info.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  dbg_messenger_create_info.pfnUserCallback = debug_messenger_callback;
  dbg_messenger_create_info.pUserData = this;
  instance_create_info.pNext = &dbg_messenger_create_info;
#endif

  ErrorCheck(vkCreateInstance(&instance_create_info, nullptr, &_instance));
}

void Renderer::_DeInitInstance()
{
  vkDestroyInstance(_instance, nullptr);
  _instance = nullptr;
}

void Renderer::_InitDevice()
{
  {
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(_instance, &gpu_count, nullptr);
    std::vector<VkPhysicalDevice> gpu_list(gpu_count);
    vkEnumeratePhysicalDevices(_instance, &gpu_count, gpu_list.data());
    _gpu = gpu_list[0];
    vkGetPhysicalDeviceProperties(_gpu, &_gpu_properties);
    vkGetPhysicalDeviceMemoryProperties(_gpu, &_gpu_memory_properties);


    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceFeatures(_gpu, &deviceFeatures);

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    VkPhysicalDeviceMaintenance4FeaturesKHR maintenance4Features = {};
    maintenance4Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR;
    maintenance4Features.maintenance4 = VK_TRUE;

    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &maintenance4Features;

    vkGetPhysicalDeviceFeatures2(_gpu, &deviceFeatures2);

    if (deviceFeatures.tessellationShader ==  VK_TRUE && deviceFeatures.geometryShader == VK_TRUE ) {
      canUseTess = true;
    }
    else {
      canUseTess = false;
    }
  }
  {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> family_property_list(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, family_property_list.data());

    bool found = false;
    for (uint32_t i = 0; i < family_count; ++i) {
      if (family_property_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        found = true;
        _graphics_family_index = i;
      }
      if (family_property_list[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
        found = true;
        _compute_family_index = i;
      }

    }
    if (!found) {
      assert(0 && "Vulkan ERROR: Queue family supporting graphics not found.");
      std::exit(-1);
    }
  }

  if (_compute_family_index == _graphics_family_index) {

    float queue_priorities[]{ 0.5f, 1.0f };
    VkDeviceQueueCreateInfo device_queue_create_info{};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = _graphics_family_index;
    device_queue_create_info.queueCount = 2;
    device_queue_create_info.pQueuePriorities = queue_priorities;

    VkDeviceQueueCreateInfo infos[] = { device_queue_create_info };

    VkPhysicalDeviceFeatures features = {};
    if (canUseTess) {
      features.tessellationShader = VK_TRUE;
      features.geometryShader = VK_TRUE;
    }
#ifdef _WINDOWS
    features.samplerAnisotropy = VK_TRUE;
#endif
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pEnabledFeatures = &features;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = infos;
    device_create_info.enabledExtensionCount = _device_extensions.size();
    device_create_info.ppEnabledExtensionNames = _device_extensions.data();

    ErrorCheck(vkCreateDevice(_gpu, &device_create_info, nullptr, &_device));
    vkGetDeviceQueue(_device, _graphics_family_index, 0, &_queue);
    vkGetDeviceQueue(_device, _graphics_family_index, 1, &_queueCompute);


  }
  else {
    float queue_priorities[]{ 0.9f };
    VkDeviceQueueCreateInfo device_queue_create_info{};
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = _graphics_family_index;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = queue_priorities;

    float compute_queue_priorities[]{ 1.0f };
    VkDeviceQueueCreateInfo compute_device_queue_create_info{};
    compute_device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    compute_device_queue_create_info.queueFamilyIndex = _compute_family_index;
    compute_device_queue_create_info.queueCount = 1;
    compute_device_queue_create_info.pQueuePriorities = queue_priorities;

    VkDeviceQueueCreateInfo infos[] = { device_queue_create_info , compute_device_queue_create_info, };

    VkPhysicalDeviceFeatures features = {};
    if (canUseTess) {
      features.tessellationShader = VK_TRUE;
      features.geometryShader = VK_TRUE;
    }
#ifdef _WINDOWS
    // I don't know hwy this is needed. but validation error happed when samplerAnisotropy is false
    features.samplerAnisotropy = VK_TRUE;
#endif
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pEnabledFeatures = &features;
    device_create_info.queueCreateInfoCount = 2;
    device_create_info.pQueueCreateInfos = infos;
    device_create_info.enabledExtensionCount = _device_extensions.size();
    device_create_info.ppEnabledExtensionNames = _device_extensions.data();

    ErrorCheck(vkCreateDevice(_gpu, &device_create_info, nullptr, &_device));
    vkGetDeviceQueue(_device, _graphics_family_index, 0, &_queue);
    vkGetDeviceQueue(_device, _compute_family_index, 0, &_queueCompute);
  }


  
}

void Renderer::_DeInitDevice()
{
  vkDestroyDevice(_device, nullptr);
  _device = nullptr;
}

#if BUILD_ENABLE_VULKAN_DEBUG

void backtraceToLogcat();

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
  VkDebugReportFlagsEXT		flags,
  VkDebugReportObjectTypeEXT	obj_type,
  uint64_t					src_obj,
  size_t						location,
  int32_t						msg_code,
  const char *				layer_prefix,
  const char *				msg,
  void *						user_data
)
{
  std::ostringstream stream;
  stream << "VKDBG: ";
  if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
    stream << "INFO: ";
  }
  if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    stream << "WARNING: ";
  }
  if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
    stream << "PERFORMANCE: ";
  }
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    stream << "ERROR: ";
  }
  if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
    stream << "DEBUG: ";
  }
  stream << "@[" << layer_prefix << "]: ";
  stream << msg << std::endl;
  std::cout << stream.str();

#if defined(ANDROID)
  LOGE("%s", stream.str().c_str());
  backtraceToLogcat();

  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    abort();
  }
#endif

#if defined( _WIN32 )
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    //MessageBox( NULL, stream.str().c_str(), _T("Vulkan Error!"), 0 );
  }
#endif

  return false;
}


static VkBool32 demo_check_layers(uint32_t check_count, char **check_names, uint32_t layer_count, VkLayerProperties *layers) {
  for (uint32_t i = 0; i < check_count; i++) {
    VkBool32 found = 0;
    for (uint32_t j = 0; j < layer_count; j++) {
      if (!strcmp(check_names[i], layers[j].layerName)) {
        found = 1;
        break;
      }
    }
    if (!found) {
      fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
      return 0;
    }
  }
  return 1;
}


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void Renderer::_SetupDebug()
{

  _debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
  _debug_callback_create_info.pfnCallback = VulkanDebugCallback;
  _debug_callback_create_info.flags =
    VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
    VK_DEBUG_REPORT_WARNING_BIT_EXT |
    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
    VK_DEBUG_REPORT_ERROR_BIT_EXT |
    /*VK_DEBUG_REPORT_DEBUG_BIT_EXT |*/
    0;

  int enabled_extension_count = 0;
  int enabled_layer_count = 0;

  VkResult err;
  uint32_t instance_extension_count = 0;
  uint32_t instance_layer_count = 0;
  char *instance_validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

  VkBool32 validation_found = 0;
  err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
  assert(!err);

  if (instance_layer_count > 0) {
     VkLayerProperties *instance_layers = (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * instance_layer_count);
     err = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);
     assert(!err);

     validation_found = demo_check_layers(ARRAY_SIZE(instance_validation_layers), instance_validation_layers,
        instance_layer_count, instance_layers);
      if (validation_found) {
        enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
        //enabled_layers[0] = "VK_LAYER_KHRONOS_validation";
      }
      free(instance_layers);
    }

    if (!validation_found) {
/*
      ERR_EXIT(
        "vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
        "Please look at the Getting Started guide for additional information.\n",
        "vkCreateInstance Failure");
*/
      exit(-1);
    }
    

  _instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  _instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  #if defined(ANDROID)
  #else
  _instance_layers.push_back("VK_LAYER_KHRONOS_validation");
  #endif

   //_instance_layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
  /*
    _instance_layers.push_back( "VK_LAYER_LUNARG_threading" );
    _instance_layers.push_back( "VK_LAYER_GOOGLE_threading" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_draw_state" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_image" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_mem_tracker" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_object_tracker" );
    _instance_layers.push_back( "VK_LAYER_LUNARG_param_checker" );
    _instance_extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
  */

}

PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT;
PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabelEXT;
PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;
VkDebugUtilsMessengerEXT dbg_messenger;

PFN_vkCreateDebugReportCallbackEXT		fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT		fvkDestroyDebugReportCallbackEXT = nullptr;


void vkDebugNameObject(VkDevice device, VkObjectType object_type, uint64_t vulkan_handle, const char *format, ...) {
 #if  _DEBUG_
  VkResult err;
  char name[1024];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(name, sizeof(name), format, argptr);
  va_end(argptr);
  name[sizeof(name) - 1] = '\0';

  VkDebugUtilsObjectNameInfoEXT obj_name;
  obj_name.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
  obj_name.pNext = NULL;
  obj_name.objectType = object_type;
  obj_name.objectHandle = vulkan_handle;
  obj_name.pObjectName = name;
  
  err = SetDebugUtilsObjectNameEXT(device, &obj_name);
  assert(!err);
#endif
}


void Renderer::_InitDebug()
{

  CreateDebugUtilsMessengerEXT =
    (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
  DestroyDebugUtilsMessengerEXT =
    (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
  SubmitDebugUtilsMessageEXT =
    (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(_instance, "vkSubmitDebugUtilsMessageEXT");
  CmdBeginDebugUtilsLabelEXT =
    (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(_instance, "vkCmdBeginDebugUtilsLabelEXT");
  CmdEndDebugUtilsLabelEXT =
    (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(_instance, "vkCmdEndDebugUtilsLabelEXT");
  CmdInsertDebugUtilsLabelEXT =
    (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(_instance, "vkCmdInsertDebugUtilsLabelEXT");
  SetDebugUtilsObjectNameEXT =
    (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(_instance, "vkSetDebugUtilsObjectNameEXT");

  if (NULL == CreateDebugUtilsMessengerEXT || NULL == DestroyDebugUtilsMessengerEXT ||
    NULL == SubmitDebugUtilsMessageEXT || NULL == CmdBeginDebugUtilsLabelEXT ||
    NULL == CmdEndDebugUtilsLabelEXT || NULL == CmdInsertDebugUtilsLabelEXT ||
    NULL == SetDebugUtilsObjectNameEXT) {

    fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
    fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");
    if (nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT) {
      assert(0 && "Vulkan ERROR: Can't fetch debug function pointers.");
      std::exit(-1);
    }

    fvkCreateDebugReportCallbackEXT(_instance, &_debug_callback_create_info, nullptr, &_debug_report);
  }
  else {
    
    CreateDebugUtilsMessengerEXT(_instance, &dbg_messenger_create_info, NULL, &dbg_messenger);
  }

  

  //	vkCreateDebugReportCallbackEXT( _instance, nullptr, nullptr, nullptr );
}

void Renderer::_DeInitDebug()
{
  if (_debug_report != VK_NULL_HANDLE) {
    fvkDestroyDebugReportCallbackEXT(_instance, _debug_report, nullptr);
    _debug_report = VK_NULL_HANDLE;
  }

  if (dbg_messenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(_instance, dbg_messenger, nullptr);
    dbg_messenger = VK_NULL_HANDLE;
  }

}

#else

void Renderer::_SetupDebug() {};
void Renderer::_InitDebug() {};
void Renderer::_DeInitDebug() {};

#endif // BUILD_ENABLE_VULKAN_DEBUG
