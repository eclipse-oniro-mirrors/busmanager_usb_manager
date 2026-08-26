/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const MASK = '***';

const SENSITIVE_KEYS: Set<string> = new Set([
  'tokenId',
  'tokenID',
  'deviceId',
  'deviceID',
  'deviceName',
  'portId',
  'portName',
  'accessory',
  'appName',
  'productName',
  'serial',
  'serialNumber',
  'sn',
  'iSerialNumber',
  'manufacturer',
  'manufacturerName',
  'model',
  'modelName',
  'vendor',
  'vendorName'
]);

function isSensitiveKey(key: string): boolean {
  return SENSITIVE_KEYS.has(key);
}

function deepMask(obj: Object): Object {
  if (obj === null || obj === undefined) {
    return obj;
  }
  if (typeof obj !== 'object') {
    return obj;
  }
  if (Array.isArray(obj)) {
    return (obj as Array<Object>).map((item: Object) => deepMask(item));
  }
  const result: Record<string, Object> = {};
  const record = obj as Record<string, Object>;
  const keys = Object.keys(record);
  for (const key of keys) {
    if (isSensitiveKey(key)) {
      result[key] = MASK;
    } else {
      result[key] = deepMask(record[key]);
    }
  }
  return result;
}

export function maskWant(want: Object): string {
  if (want === null || want === undefined) {
    return JSON.stringify(want);
  }
  try {
    const w = want as Record<string, Object>;
    const sanitized: Record<string, Object> = {};
    if (w.abilityName !== undefined) {
      sanitized['abilityName'] = w.abilityName;
    }
    if (w.bundleName !== undefined) {
      sanitized['bundleName'] = w.bundleName;
    }
    if (w.action !== undefined) {
      sanitized['action'] = w.action;
    }
    if (w.parameters !== undefined && w.parameters !== null) {
      sanitized['parameters'] = deepMask(w.parameters);
    }
    return JSON.stringify(sanitized);
  } catch (e) {
    return MASK;
  }
}

export function maskObject(obj: Object): string {
  if (obj === null || obj === undefined) {
    return JSON.stringify(obj);
  }
  try {
    if (obj instanceof Error) {
      const errInfo: Record<string, Object> = {
        name: obj.name,
        message: MASK
      };
      const code = (obj as Record<string, Object>)['code'];
      if (code !== undefined) {
        errInfo['code'] = code;
      }
      return JSON.stringify(errInfo);
    }
    return JSON.stringify(deepMask(obj));
  } catch (e) {
    return MASK;
  }
}
