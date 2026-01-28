..  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
    SPDX-License-Identifier: BSD-3-Clause-Clear

.. _core-daemon-apis:

Core Daemons APIs
=================

The Core Daemon Services provide full-featured interface access to configure and control services and apps.

+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| Service     | API guide                                  | API reference                                                 | File name            | Description                                                  |
+=============+============================================+===============================================================+======================+==============================================================+
| configTree  | :doc:`../_doxygen_rst/page_c_config`       | :ref:`le_cfg_interface.h <File le_cfg_interface.h>`           | le_cfg.api           | Functions to read and write data into the app's tree         |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| configTree  | :doc:`../_doxygen_rst/page_c_configAdmin`  | :ref:`le_cfg_interface.h <File le_cfgAdmin_interface.h>`      | le_cfgAdmin.api      | Tools to facilitate the administration of App's Trees        |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| supervisor  | :doc:`../_doxygen_rst/page_c_appCtrl`      | :ref:`le_cfg_interface.h <File le_appCtrl_interface.h>`       | le_appCtrl.api       | Control TelAF apps                                           |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| supervisor  | :doc:`../_doxygen_rst/page_c_appInfo`      | :ref:`le_cfg_interface.h <File le_appInfo_interface.h>`       | le_appInfo.api       | TelAF app info retrieval                                     |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| supervisor  | :doc:`../_doxygen_rst/page_c_framework`    |:ref:`le_cfg_interface.h <File le_framework_interface.h>`      | le_framework.api     | Control the TelAF framework                                  |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| supervisor  | :doc:`../_doxygen_rst/page_c_kernelModule` | :ref:`le_cfg_interface.h <File le_kernelModule_interface.h>`  | le_kernelModule.api  | Module load and unload                                       |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| update      | :doc:`../_doxygen_rst/page_c_update`       |:ref:`le_cfg_interface.h <File le_update_interface.h>`         | le_update.api        | Control the update daemon on the target                      |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| update      | :doc:`../_doxygen_rst/page_c_updateCtrl`   |:ref:`le_cfg_interface.h <File le_updateCtrl_interface.h>`     | le_updateCtrl.api    | Tools to facilitate the administration of the update daemon  |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| update      | :doc:`../_doxygen_rst/page_c_le_instStat`  |:ref:`le_cfg_interface.h <File le_instStat_interface.h>`       | le_instStat.api      | Notifications when apps are installed and uninstalled        |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
| watchdog    | :doc:`../_doxygen_rst/page_c_wdog`         |:ref:`le_cfg_interface.h <File le_wdog_interface.h>`           | le_wdog.api          | Monitor critical applications and services for deadlocks and |
|             |                                            |                                                               |                      | other similar faults                                         |
+-------------+--------------------------------------------+---------------------------------------------------------------+----------------------+--------------------------------------------------------------+
