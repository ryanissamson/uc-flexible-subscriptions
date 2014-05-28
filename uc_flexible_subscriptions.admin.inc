<?php

/* @file
 * Create administrative forms to update subscription renewal dates and grant files
 * from selected products by user role.
 */

function uc_role_renew_admin_form(&$form_state) {
  $form = array();
  $form['roles_to_update'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the user role(s) that you want to update'),
  );
  $form['roles_to_update']['roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('User roles'),
    // Use the role names as array keys for matching.
    '#options' => $roles = drupal_map_assoc(user_roles(TRUE)),
    '#required' => $field->required,
  );
  $form['current_information'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the current subscription renewal date range that you want to update'),
  );
  $form['current_information']['start_date'] = array(
    '#type' => 'date',
    '#title' => t('Start date.'),
    '#required' => $field->required,
  );
  $form['current_information']['end_date'] = array(
    '#type' => 'date',
    '#title' => t('End date'),
    '#required' => $field->required,
  );
  $form['new_information'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the new subscription renewal date'),
  );
  $form['new_information']['renewal_date'] = array(
    '#type' => 'date',
    '#title' => t('New renewal date'),
    '#default_value' => $date,
    '#required' => $field->required,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

function uc_role_renew_admin_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  
  // Check each row to match the role.
  foreach($values['roles'] as $role) {
    if (is_string($role)) {
      // Get the date and conver it to unix.
      $date = strtotime(implode('/', $values['renewal_date']));
      $role = str_replace(' ', '_', $role);
      db_query('update {uc_recurring_users} set next_charge=%d where data like "%%%s%%"', $date, $role );
    }
  }
  drupal_set_message(t('The subscription renewal date has been updated.'));
}

function uc_grant_file_admin_form(&$form_state) {
  // List the available products and limit the list to products with files.
  $product_query = "select ucp.nid, n.title, ucpc.name as type 
    from {uc_products} as ucp
    join {node} as n on ucp.nid = n.nid
    join {uc_file_products} as ufp on ucp.model = ufp.model
    left join {uc_product_classes} as ucpc on ucpc.pcid = n.type
    order by ucpc.name, n.title";

  $products = db_query($product_query);

  $product_list[0] = t('Select a product');

  while ($product = db_fetch_array($products)) {
    if ($product['type'] != NULL)
      $type = $product['type'];
    else {
      $type = "Product";
    }
    $product_list[$type][$product['nid']] = $product['title'];
  }
  
  $form = array();
  $form['product_select'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the product which has the files you want to grant to customers.'),
  );
  $form['product_select']['products'] = array(
    '#type' => 'select',
    '#title' => t('Products'),
    '#options' => $product_list,
  );
  $form['roles_to_update'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the customer role(s) that you want to receive the file(s)'),
  );
  $form['roles_to_update']['roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('User roles'),
    // Use the role names as array keys for matching
    '#options' => $roles = drupal_map_assoc(user_roles(TRUE)),
    '#required' => $field->required,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

function uc_grant_file_admin_form_submit($form, &$form_state) {
  $values = $form_state['values'];

  $product = $values['products'];

  if (is_numeric($product)) {
    $nid = $product;
  } 

  // Prepare the files from the matching product.
  $file = db_fetch_object(db_query('select fid, pfid from {uc_file_products} as fp
                                    join {uc_products} as p on p.nid=%d and p.model=fp.model', $nid));
  $fid = $file->fid;
  $pfid = $file->pfid;

  foreach($values['roles'] as $role) {
    if (is_string($role)) {
      // Build the list of users using the specified role.
      $result = db_query('select distinct u.uid, u.name from {users} as u
        join {users_roles} as ur on ur.uid=u.uid
        join {role} as r on r.rid=ur.rid
        where r.name like "%s"
        order by u.name', array($role));
     }
    else {
      break;
    }
  }

  while ($r = db_fetch_object($result)) {
    $user = user_load($r->uid);
    // Grant the product file(s) to the user accounts
    uc_file_user_renew($fid, $user, $pfid, $file_limits=array(), $force_overwrite = TRUE);
  }
  drupal_set_message(t('Files have been granted for the selected roles.'));
}
